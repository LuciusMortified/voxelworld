#pragma once

#ifndef VW_GFX_WORLD_H
#define VW_GFX_WORLD_H

#include <utility>
#include <vector>
#include <memory>
#include <unordered_map>
#include <future>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>

#include "vw/core.h"
#include "vw/gfx/resource/mesh.h"
#include "vw/gfx/resource/model.h"
#include "vw/gfx/world/transform.h"

namespace vw::gfx {
    class vulkan_context;
    
    using object_id = uint32;

    struct world_object {
        object_id id;
        
        std::shared_ptr<model> pmodel;
        transform transform;

        std::shared_ptr<mesh> pmesh;
        bool visible = true;
        
        std::future<mesh_data> mesh_future;
        bool mesh_dirty = true;
        
        world_object(object_id id, std::shared_ptr<model> pmodel)
            : id(id), pmodel(std::move(pmodel)) {}
    };

    struct mesh_generation_task {
        object_id id;
        std::shared_ptr<model> pmodel;
        std::promise<mesh_data> promise;
        
        mesh_generation_task(object_id id, std::shared_ptr<model> pmodel)
            : id(id), pmodel(std::move(pmodel)) {}
    };

    class world {
    public:
        explicit world(vulkan_context& context);
        ~world();

        world(const world&) = delete;
        world& operator=(const world&) = delete;
        world(world&&) = delete;
        world& operator=(world&&) = delete;

        object_id add_object(
            std::shared_ptr<model> model, 
            const vec3f& position = {0,0,0},
            const vec3f& rotation = {0,0,0},
            const vec3f& scale = {1,1,1}
        );
        void remove_object(object_id id);
        void clear();

        std::shared_ptr<world_object> get_object(object_id id);
        std::shared_ptr<const world_object> get_object(object_id id) const;
        const std::vector<std::shared_ptr<world_object>>& get_objects() const { return objects_; }
        size_t get_object_count() const { return objects_.size(); }

        void set_object_position(object_id id, const vec3f& position);
        void set_object_rotation(object_id id, const vec3f& rotation);
        void set_object_scale(object_id id, const vec3f& scale);
        void set_object_transform(object_id id, const transform& transform);

        void translate_object(object_id id, const vec3f& offset);
        void rotate_object(object_id id, const vec3f& angles);
        void scale_object(object_id id, const vec3f& factor);

        void set_object_model(object_id id, std::shared_ptr<model> new_model);
        std::shared_ptr<model> get_object_model(object_id id) const;

        void set_object_visible(object_id id, bool visible);
        bool is_object_visible(object_id id) const;

        void update_meshes();

        object_id get_next_object_id() { return next_object_id_++; }
        bool object_exists(object_id id) const;

    private:
        vulkan_context* context_;

        std::vector<std::shared_ptr<world_object>> objects_;
        std::unordered_map<object_id, std::weak_ptr<world_object>> object_map_;
        object_id next_object_id_ = 1;

        std::thread gen_thread_;
        std::queue<std::unique_ptr<mesh_generation_task>> gen_queue_;
        std::mutex gen_mutex_;
        std::condition_variable gen_cv_;
        bool gen_running_ = true;

        void mark_object_mesh_dirty(object_id id);
        void update_object_mesh(std::shared_ptr<world_object> obj);

        void gen_thread_function();
        void process_completed_meshes();
    };
} // namespace vw::gfx

#endif // VW_GFX_WORLD_H
