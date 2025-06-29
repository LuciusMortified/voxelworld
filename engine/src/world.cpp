#include <voxel/world.h>
#include <voxel/vulkan_context.h>
#include <voxel/mesh.h>
#include <chrono>

namespace voxel {

world::world(std::shared_ptr<vulkan_context> context) 
    : context_(context), worker_running_(true) {
    // Запускаем рабочий поток для генерации мешей
    worker_thread_ = std::thread(&world::worker_thread_function, this);
}

world::~world() {
    // Останавливаем рабочий поток
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        worker_running_ = false;
    }
    task_cv_.notify_one();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

object_id world::add_object(
    std::shared_ptr<model> model, 
    const vec3f& position,
    const vec3f& rotation,
    const vec3f& scale
) {
    object_id id = get_next_object_id();
    
    auto obj = std::make_shared<world_object>(id, model);
    obj->transform.set_position(position);
    obj->transform.set_rotation(rotation);
    obj->transform.set_scale(scale);
    
    // Добавляем в вектор и карту
    objects_.push_back(obj);
    object_map_[id] = obj;
    
    // Запускаем асинхронную генерацию меша
    update_object_mesh(obj);
    
    return id;
}

void world::remove_object(object_id id) {
    auto it = object_map_.find(id);
    if (it != object_map_.end()) {
        // Удаляем из карты
        object_map_.erase(it);
        
        // Удаляем из вектора
        objects_.erase(
            std::remove_if(objects_.begin(), objects_.end(),
                [id](const std::shared_ptr<world_object>& obj) {
                    return obj->id == id;
                }),
            objects_.end()
        );
    }
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

// Методы для работы с трансформациями
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

// Методы для работы с моделями
void world::set_object_model(object_id id, std::shared_ptr<model> new_model) {
    if (auto obj = get_object(id)) {
        obj->pmodel = new_model;
        obj->mesh_dirty = true;
    }
}

std::shared_ptr<model> world::get_object_model(object_id id) const {
    if (auto obj = get_object(id)) {
        return obj->pmodel;
    }
    return nullptr;
}

// Методы для работы с видимостью
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

// Методы для рендеринга
void world::update_meshes() {
    // Обрабатываем завершенные задачи генерации мешей
    process_completed_meshes();
    
    // Запускаем генерацию для объектов с mesh_dirty = true
    for (auto& obj : objects_) {
        if (obj->mesh_dirty) {
            update_object_mesh(obj);
        }
    }
}

// Утилиты
bool world::object_exists(object_id id) const {
    return object_map_.find(id) != object_map_.end();
}

// Внутренние методы
void world::mark_object_mesh_dirty(object_id id) {
    if (auto obj = get_object(id)) {
        obj->mesh_dirty = true;
    }
}

void world::update_object_mesh(std::shared_ptr<world_object> obj) {
    if (!obj || !context_ || !obj->pmodel || obj->mesh_dirty == false) return;
    
    // Создаем задачу генерации меша
    auto task = std::make_unique<mesh_generation_task>(obj->id, obj->pmodel);
    auto future = task->promise.get_future();
    
    // Сохраняем future в объекте
    obj->mesh_future = std::move(future);
    
    // Добавляем задачу в очередь
    {
        std::lock_guard<std::mutex> lock(task_mutex_);
        task_queue_.push(std::move(task));
    }
    task_cv_.notify_one();
    
    // Помечаем объект как ожидающий генерации меша
    obj->mesh_dirty = false;
}

void world::worker_thread_function() {
    while (true) {
        std::unique_ptr<mesh_generation_task> task;
        
        // Ждем задачи
        {
            std::unique_lock<std::mutex> lock(task_mutex_);
            task_cv_.wait(lock, [this] { 
                return !task_queue_.empty() || !worker_running_; 
            });
            
            if (!worker_running_ && task_queue_.empty()) {
                break;
            }
            
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
        }
        
        if (task) {
            try {
                // Генерируем данные меша в отдельном потоке (без Vulkan буферов)
                mesh_data data = greedy_mesh_generator::generate_mesh_data(task->pmodel);
                
                // Возвращаем результат
                task->promise.set_value(std::move(data));
            } catch (const std::exception& e) {
                // В случае ошибки возвращаем пустые данные
                task->promise.set_value(mesh_data());
            }
        }
    }
}

void world::process_completed_meshes() {
    // Проверяем завершенные задачи генерации мешей
    for (auto& obj : objects_) {
        if (obj->mesh_future.valid()) {
            // Проверяем, готов ли результат
            if (obj->mesh_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                try {
                    // Получаем данные меша
                    mesh_data data = obj->mesh_future.get();
                    
                    // Создаем меш из данных
                    obj->pmesh = std::make_shared<mesh>(context_);
                    obj->pmesh->set_mesh_data(data);
                } catch (const std::exception& e) {
                    // Если генерация не удалась, очищаем меш
                    obj->pmesh = nullptr;
                }
            }
        }
    }
}

} 