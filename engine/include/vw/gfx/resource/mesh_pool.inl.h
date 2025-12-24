#pragma once

#ifndef VW_GFX_RESOURCE_MESH_POOL_INL_H
#define VW_GFX_RESOURCE_MESH_POOL_INL_H

#include <algorithm>
#include <functional>

#include "vw/gfx/model/model.h"
#include "vw/gfx/render/vulkan_context.h"
#include "vw/gfx/resource/mesh.h"
#include "vw/gfx/resource/mesh_pool.h"

namespace vw::gfx {

inline mesh_pool::mesh_pool(
    vulkan_context& context
)
    : context_{&context}, gen_running_(true) {
    gen_thread_ = std::thread(&mesh_pool::gen_thread_function, this);
}

inline mesh_pool::~mesh_pool() {
    {
        std::lock_guard lock(gen_mutex_);
        gen_running_ = false;
    }
    gen_cv_.notify_one();

    if (gen_thread_.joinable()) {
        gen_thread_.join();
    }
}

[[nodiscard]] inline auto mesh_pool::has(
    const model_identity& identity
) const -> bool {
    return meshes_.contains(identity);
}

[[nodiscard]] inline auto mesh_pool::is_pending(
    const model_identity& identity
) const -> bool {
    return pending_meshes_.contains(identity);
}

inline void mesh_pool::request_mesh(
    const std::shared_ptr<model>& model_ptr
) {
    model_identity identity = model_ptr->get_identity();

    if (has(identity) || is_pending(identity)) {
        return;
    }

    log::debug(
        lc_,
        "Requesting mesh generation for model {}.{} with size ({},{},{})",
        identity.index,
        identity.generation,
        model_ptr->width(),
        model_ptr->height(),
        model_ptr->depth()
    );

    {
        auto task   = std::make_unique<mesh_generation_task>(identity, model_ptr);
        auto future = task->promise.get_future();

        std::lock_guard lock(gen_mutex_);
        pending_meshes_[identity] = std::move(future);
        gen_queue_.push(std::move(task));
    }
    gen_cv_.notify_one();
}

[[nodiscard]] inline auto mesh_pool::get(
    const model_identity& identity
) const -> std::shared_ptr<mesh> {
    auto iter = meshes_.find(identity);
    return iter != meshes_.end() ? iter->second : nullptr;
}

inline void mesh_pool::process_completed() {
    for (auto iter = pending_meshes_.begin(); iter != pending_meshes_.end();) {
        auto status = iter->second.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            auto identity     = iter->first;
            auto data         = iter->second.get();

            log::debug(
                lc_,
                "Completed mesh generation for model {}.{} vertices {} indices {}",
                identity.index,
                identity.generation,
                data.vertices.size(),
                data.indices.size()
            );

            meshes_[identity] = std::make_shared<mesh>(std::move(data));

            iter = pending_meshes_.erase(iter);
        } else {
            ++iter;
        }
    }
}

inline void mesh_pool::gen_thread_function() {
    while (true) {
        std::unique_ptr<mesh_generation_task> task;

        {
            std::unique_lock lock(gen_mutex_);
            gen_cv_.wait(lock, [this] { return !gen_queue_.empty() || !gen_running_; });

            if (!gen_running_ && gen_queue_.empty()) {
                break;
            }

            if (!gen_queue_.empty()) {
                task = std::move(gen_queue_.front());
                gen_queue_.pop();
            }
        }

        if (task) {
            try {
                mesh data = greedy_mesh_generator::generate_mesh_data(task->model_ptr);
                task->promise.set_value(std::move(data));
            } catch (const std::exception& e) {
                task->promise.set_exception(std::current_exception());
            }
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_RESOURCE_MESH_POOL_INL_H
