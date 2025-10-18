#include "vw/gfx/world/world.h"

#include <chrono>
#include <iostream>

#include "vw/gfx/render/vulkan_context.h"
#include "vw/gfx/resource/mesh.h"

namespace vw::gfx {

world::world(vulkan_context& context) : context_(&context), transform_system_(registry_), hierarchy_system_(registry_, transform_system_) {
    gen_thread_ = std::thread(&world::gen_thread_function, this);
}

world::~world() {
    {
        std::lock_guard lock(gen_mutex_);
        gen_running_ = false;
    }
    gen_cv_.notify_one();

    if (gen_thread_.joinable()) {
        gen_thread_.join();
    }
}

object_id world::add_object(
    std::shared_ptr<model> model,
    const vec3f& position,
    const vec3f& rotation,
    const vec3f& scale,
    const vec3f& origin
) {
    object_id id = get_next_object_id();

    auto obj = std::make_shared<world_object>(id, model);
    obj->transform.set_position(position);
    obj->transform.set_rotation(rotation);
    obj->transform.set_scale(scale);
    obj->transform.set_origin(origin);

    objects_.push_back(obj);
    object_map_[id] = obj;

    return id;
}

void world::remove_object(object_id id) {
    object_map_.erase(id);
    std::erase_if(objects_, [id](const std::shared_ptr<world_object>& obj) {
        return obj->id == id;
    });
}

void world::clear() {
    objects_.clear();
    object_map_.clear();
}

std::shared_ptr<world_object> world::get_object(object_id id) {
    auto it = object_map_.find(id);
    if (it != object_map_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

std::shared_ptr<const world_object> world::get_object(object_id id) const {
    auto it = object_map_.find(id);
    if (it != object_map_.end()) {
        return it->second.lock();
    }
    return nullptr;
}

void world::set_object_position(object_id id, const vec3f& position) {
    if (auto obj = get_object(id)) {
        obj->transform.set_position(position);
    }
}

void world::set_object_rotation(object_id id, const vec3f& rotation) {
    if (auto obj = get_object(id)) {
        obj->transform.set_rotation(rotation);
    }
}

void world::set_object_scale(object_id id, const vec3f& scale) {
    if (auto obj = get_object(id)) {
        obj->transform.set_scale(scale);
    }
}

void world::set_object_transform(object_id id, const transform& transform_data) {
    if (auto obj = get_object(id)) {
        obj->transform = transform_data;
    }
}

void world::translate_object(object_id id, const vec3f& offset) {
    if (auto obj = get_object(id)) {
        obj->transform.translate(offset);
    }
}

void world::rotate_object(object_id id, const vec3f& angles) {
    if (auto obj = get_object(id)) {
        obj->transform.rotate(angles);
    }
}

void world::scale_object(object_id id, const vec3f& factor) {
    if (auto obj = get_object(id)) {
        obj->transform.scale(factor);
    }
}

void world::set_object_model(object_id id, std::shared_ptr<model> new_model) {
    if (auto obj = get_object(id)) {
        obj->pmodel     = new_model;
        obj->mesh_dirty = true;
    }
}

std::shared_ptr<model> world::get_object_model(object_id id) const {
    if (auto obj = get_object(id)) {
        return obj->pmodel;
    }
    return nullptr;
}

void world::set_object_visible(object_id id, bool visible) {
    if (auto obj = get_object(id)) {
        obj->visible = visible;
    }
}

bool world::is_object_visible(object_id id) const {
    if (auto obj = get_object(id)) {
        return obj->visible;
    }
    return false;
}

void world::update_meshes() {
    process_completed_meshes();

    for (auto& obj : objects_) {
        if (obj->mesh_dirty) {
            update_object_mesh(obj);
        }
    }
}

void world::update() {
    transform_system_.update();
}

bool world::object_exists(object_id id) const {
    return object_map_.contains(id);
}

void world::mark_object_mesh_dirty(object_id id) {
    if (const auto obj = get_object(id)) {
        obj->mesh_dirty = true;
    }
}

void world::update_object_mesh(std::shared_ptr<world_object> obj) {
    if (!obj || !context_ || !obj->pmodel || !obj->mesh_dirty) {
        return;
    }

    auto task   = std::make_unique<mesh_generation_task>(obj->id, obj->pmodel);
    auto future = task->promise.get_future();

    obj->mesh_future = std::move(future);

    {
        std::lock_guard<std::mutex> lock(gen_mutex_);
        gen_queue_.push(std::move(task));
    }
    gen_cv_.notify_one();

    obj->mesh_dirty = false;
}

void world::gen_thread_function() {
    while (gen_running_) {
        std::unique_ptr<mesh_generation_task> task;

        {
            std::unique_lock<std::mutex> lock(gen_mutex_);
            gen_cv_.wait(lock, [this] { return !gen_queue_.empty() || !gen_running_; });

            if (!gen_queue_.empty()) {
                task = std::move(gen_queue_.front());
                gen_queue_.pop();
            }
        }

        if (task) {
            try {
                mesh_data data = greedy_mesh_generator::generate_mesh_data(task->pmodel);

                task->promise.set_value(std::move(data));
            } catch (const std::exception& e) {
                std::cerr << "Error generating mesh: " << e.what() << std::endl;

                task->promise.set_value(mesh_data());
            }
        }
    }
}

void world::process_completed_meshes() {
    for (auto& obj : objects_) {
        if (!obj->mesh_future.valid()) {
            continue;
        }

        auto status = obj->mesh_future.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            try {
                mesh_data data = obj->mesh_future.get();

                obj->pmesh = std::make_shared<mesh>(*context_);
                obj->pmesh->set_mesh_data(data);
            } catch (const std::exception& e) {
                std::cerr << "Error generating mesh: " << e.what() << std::endl;
                obj->pmesh = nullptr;
            }
        }
    }
}

}  // namespace vw::gfx