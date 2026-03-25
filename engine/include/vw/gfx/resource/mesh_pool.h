#pragma once

#ifndef VW_GFX_MESH_POOL_H
#define VW_GFX_MESH_POOL_H

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "vw/gfx/model/model_identity.h"
#include "vw/gfx/resource/mesh.h"

namespace vw::gfx {

class vulkan_context;
class model;

struct mesh_generation_task {
    model_identity identity;
    std::weak_ptr<model> model_ref;
    std::promise<mesh> promise;

    mesh_generation_task(
        model_identity identity, std::weak_ptr<model> model_ref
    )
        : identity(identity), model_ref(std::move(model_ref)) {}
};

class mesh_pool final {
public:
    explicit mesh_pool(vulkan_context& context, const block_registry& registry);
    ~mesh_pool();

    mesh_pool(const mesh_pool&)                    = delete;
    auto operator=(const mesh_pool&) -> mesh_pool& = delete;
    mesh_pool(mesh_pool&&)                         = delete;
    auto operator=(mesh_pool&&) -> mesh_pool&      = delete;

    [[nodiscard]] auto has(const model_identity& identity) const -> bool;
    [[nodiscard]] auto is_pending(const model_identity& identity) const -> bool;
    void request_mesh(const std::shared_ptr<model>& model_ptr);
    [[nodiscard]] auto get(const model_identity& identity) const -> std::shared_ptr<mesh>;
    void remove(const model_identity& identity);
    void evict(const model_identity& identity);
    void process_completed();
    [[nodiscard]] auto get_pending_count() const -> uint32;

private:
    void gen_thread_function();
    void sweep_orphaned_();

    vulkan_context* context_;
    const block_registry* registry_;
    std::unordered_map<model_identity, std::shared_ptr<mesh>> meshes_;
    std::unordered_map<model_identity, std::weak_ptr<model>> model_refs_;
    std::unordered_map<model_identity, std::future<mesh>> pending_meshes_;
    std::unordered_set<uint32> pending_indices_;

    std::vector<std::thread> gen_threads_;
    std::queue<std::unique_ptr<mesh_generation_task>> gen_queue_;
    std::mutex gen_mutex_;
    std::condition_variable gen_cv_;
    bool gen_running_ = true;
    uint32 sweep_counter_ = 0;
    static constexpr uint32 sweep_interval_ = 60;

    static constexpr log::log_category lc_{"mesh_pool"};
};

}  // namespace vw::gfx

#include "vw/gfx/resource/mesh_pool.inl.h"

#endif  // VW_GFX_MESH_POOL_H