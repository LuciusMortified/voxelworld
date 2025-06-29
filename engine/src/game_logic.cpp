#include <voxel/game_logic.h>
#include <voxel/engine.h>
#include <voxel/window.h>
#include <voxel/camera_controller.h>

namespace voxel {

    base_game_logic::base_game_logic() {
        // По умолчанию создаем FPS контроллер
        camera_controller_ = std::make_unique<fps_camera_controller>();
    }

    void base_game_logic::initialize(std::shared_ptr<engine> engine) {
        // Сохраняем weak_ptr ссылку на движок
        engine_ = engine;
        
        // Инициализируем контроллер камеры, если он есть
        if (camera_controller_) {
            camera_controller_->initialize(engine->get_window(), engine->get_camera());
        }
    }

    void base_game_logic::update(float delta_time) {
        // Обновляем контроллер камеры
        if (camera_controller_) {
            camera_controller_->update(delta_time);
        }
    }

} // namespace voxel 