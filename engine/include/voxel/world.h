#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <future>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>

#include <voxel/types.h>
#include <voxel/model.h>
#include <voxel/mesh.h>
#include <voxel/transform.h>

namespace voxel {
    class vulkan_context;

    // Структура для хранения размещенного объекта в мире
    struct world_object {
        object_id id;                 // Уникальный идентификатор
        std::shared_ptr<model> pmodel; // Указатель на воксельную модель
        transform transform;          // Трансформация объекта
        std::shared_ptr<mesh> pmesh;   // Кэшированный меш
        bool mesh_dirty = true;       // Флаг необходимости пересоздания меша
        bool visible = true;          // Видимость объекта
        std::future<mesh_data> mesh_future; // Future для асинхронной генерации
        
        world_object(object_id id, std::shared_ptr<model> pmodel)
            : id(id), pmodel(pmodel) {}
    };

    // Задача генерации меша
    struct mesh_generation_task {
        object_id id;
        std::shared_ptr<model> pmodel;
        std::promise<mesh_data> promise;
        
        mesh_generation_task(object_id id, std::shared_ptr<model> pmodel)
            : id(id), pmodel(pmodel) {}
    };

    class world {
    public:
        world(std::shared_ptr<vulkan_context> context);
        ~world();

        // Запретить копирование
        world(const world&) = delete;
        world& operator=(const world&) = delete;

        // Разрешить перемещение
        world(world&&) = delete;
        world& operator=(world&&) = delete;

        // Основные методы для работы с объектами
        object_id add_object(
            std::shared_ptr<model> model, 
            const vec3f& position = {0,0,0},
            const vec3f& rotation = {0,0,0},
            const vec3f& scale = {1,1,1}
        );
        void remove_object(object_id id);
        void clear();

        // Получение объектов
        std::shared_ptr<world_object> get_object(object_id id);
        std::shared_ptr<const world_object> get_object(object_id id) const;
        const std::vector<std::shared_ptr<world_object>>& get_objects() const { return objects_; }
        size_t get_object_count() const { return objects_.size(); }

        // Методы для работы с трансформациями
        void set_object_position(object_id id, const vec3f& position);
        void set_object_rotation(object_id id, const vec3f& rotation);
        void set_object_scale(object_id id, const vec3f& scale);
        void set_object_transform(object_id id, const transform& transform);

        void translate_object(object_id id, const vec3f& offset);
        void rotate_object(object_id id, const vec3f& angles);
        void scale_object(object_id id, const vec3f& factor);

        // Методы для работы с моделями
        void set_object_model(object_id id, std::shared_ptr<model> new_model);
        std::shared_ptr<model> get_object_model(object_id id) const;

        // Методы для работы с видимостью
        void set_object_visible(object_id id, bool visible);
        bool is_object_visible(object_id id) const;

        // Методы для рендеринга
        void update_meshes(); // Пересоздает меши для объектов с mesh_dirty = true
        const std::vector<std::shared_ptr<world_object>>& get_renderable_objects() const { return objects_; }

        // Утилиты
        object_id get_next_object_id() { return next_object_id_++; }
        bool object_exists(object_id id) const;

    private:
        std::shared_ptr<vulkan_context> context_;
        std::vector<std::shared_ptr<world_object>> objects_;
        std::unordered_map<object_id, std::weak_ptr<world_object>> object_map_; // Быстрый поиск по ID
        object_id next_object_id_ = 1;

        // Система асинхронной генерации мешей
        std::thread worker_thread_;
        std::queue<std::unique_ptr<mesh_generation_task>> task_queue_;
        std::mutex task_mutex_;
        std::condition_variable task_cv_;
        bool worker_running_ = true;

        // Внутренние методы
        void mark_object_mesh_dirty(object_id id);
        void update_object_mesh(std::shared_ptr<world_object> obj);
        void worker_thread_function();
        void process_completed_meshes();
    };
} 