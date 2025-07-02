#pragma once
#include <memory>
#include <optional>
#include <functional>
#include <stdexcept>

#include <voxel/camera_controller.h>
#include <voxel/events.h>
#include <voxel/input.h>

namespace voxel {
    class engine;
    class window;
    class camera;

    // Интерфейс для игровой логики
    class game_logic {
    public:
        virtual ~game_logic() = default;
        
        // Основные методы жизненного цикла игры
        virtual void initialize(std::shared_ptr<engine> engine) {}
        virtual void update(float delta_time) {}
        virtual void render() {}
        virtual void cleanup() {}
    };

    // Базовая реализация с дополнительной функциональностью
    class base_game_logic : public game_logic {
    public:
        base_game_logic();
        virtual ~base_game_logic() = default;
        
        void initialize(std::shared_ptr<engine> engine) override;
        void update(float delta_time) override;
        
        // Методы для работы с контроллером камеры
        void set_camera_controller(std::unique_ptr<camera_controller> controller) {
            camera_controller_ = std::move(controller);
        }
        
        camera_controller* get_camera_controller() { return camera_controller_.get(); }
        const camera_controller* get_camera_controller() const { return camera_controller_.get(); }
        
    protected:
        // Получение shared_ptr на движок
        std::shared_ptr<engine> get_engine() { 
            if (auto engine = engine_.lock()) {
                return engine;
            }
            throw std::runtime_error("Engine not available");
        }
        std::shared_ptr<const engine> get_engine() const { 
            if (auto engine = engine_.lock()) {
                return engine;
            }
            throw std::runtime_error("Engine not available");
        }
        
        // Проверка доступности компонентов
        bool is_engine_available() const { return !engine_.expired(); }
        
        std::unique_ptr<camera_controller> camera_controller_;
        
    private:
        std::weak_ptr<engine> engine_;
    };
} 