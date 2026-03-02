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
    : context_{&context} {
    auto count = std::min(std::thread::hardware_concurrency(), 4u);
    if (count == 0) {
        count = 1;
    }
    for (uint32 i = 0; i < count; ++i) {
        gen_threads_.emplace_back(&mesh_pool::gen_thread_function, this);
    }
}

inline mesh_pool::~mesh_pool() {
    {
        std::scoped_lock lock(gen_mutex_);
        gen_running_ = false;
    }
    gen_cv_.notify_all();

    for (auto& t : gen_threads_) {
        if (t.joinable()) {
            t.join();
        }
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

    model_refs_[identity] = model_ptr;

    {
        auto task   = std::make_unique<mesh_generation_task>(identity, model_ptr);
        auto future = task->promise.get_future();

        std::scoped_lock lock(gen_mutex_);
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

inline void mesh_pool::remove(
    const model_identity& identity
) {
    meshes_.erase(identity);
    model_refs_.erase(identity);
    pending_meshes_.erase(identity);
}

inline void mesh_pool::sweep_orphaned_() {
    for (auto it = model_refs_.begin(); it != model_refs_.end();) {
        if (it->second.expired()) {
            meshes_.erase(it->first);
            pending_meshes_.erase(it->first);
            it = model_refs_.erase(it);
        } else {
            ++it;
        }
    }
}

inline void mesh_pool::process_completed() {
    if (++sweep_counter_ >= sweep_interval_) {
        sweep_orphaned_();
        sweep_counter_ = 0;
    }

    for (auto iter = pending_meshes_.begin(); iter != pending_meshes_.end();) {
        const auto status = iter->second.wait_for(std::chrono::seconds(0));
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

inline auto mesh_pool::get_pending_count() const -> uint32 {
    return static_cast<uint32>(pending_meshes_.size());
}

inline void mesh_pool::gen_thread_function() {
    greedy_mesh_storage storage;

    while (true) {
        std::unique_ptr<mesh_generation_task> task;

        {
            std::unique_lock lock(gen_mutex_);
            gen_cv_.wait(lock, [this] -> bool { return !gen_queue_.empty() || !gen_running_; });

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
                mesh data = greedy_mesh_generator::generate_mesh_data(storage, *task->model_ptr);
                task->promise.set_value(std::move(data));
            } catch (const std::exception& e) {
                task->promise.set_exception(std::current_exception());
            }
        }
    }
}

}  // namespace vw::gfx

#endif  // VW_GFX_RESOURCE_MESH_POOL_INL_H
