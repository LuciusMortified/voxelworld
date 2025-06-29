#pragma once
#include <string_view>
#include <memory>
#include <chrono>

#include "window.h"
#include "vulkan_context.h"
#include "renderer.h"
#include "camera.h"
#include "game_logic.h"
#include "events.h"
#include "world.h"

namespace voxel {
    class engine : public std::enable_shared_from_this<engine> {
    public:
        engine(int width, int height, std::string_view title);
        ~engine();

        // Запретить копирование
        engine(const engine&) = delete;
        engine& operator=(const engine&) = delete;

        // Основные методы
        void run(std::unique_ptr<game_logic> logic);
        void shutdown();
        bool should_exit() const { return !running_; }

        // Геттеры - возвращают shared_ptr для гибкости
        std::shared_ptr<window> get_window() { return window_; }
        std::shared_ptr<window> get_window() const { return window_; }
        
        std::shared_ptr<vulkan_context> get_vulkan_context() { return vulkan_context_; }
        std::shared_ptr<vulkan_context> get_vulkan_context() const { return vulkan_context_; }
        
        std::shared_ptr<renderer> get_renderer() { return renderer_; }
        std::shared_ptr<renderer> get_renderer() const { return renderer_; }
        
        std::shared_ptr<camera> get_camera() { return camera_; }
        std::shared_ptr<camera> get_camera() const { return camera_; }

        // Методы для работы с миром
        std::shared_ptr<world> get_world() { return world_; }
        std::shared_ptr<world> get_world() const { return world_; }

        // Методы для работы с игровой логикой
        void set_game_logic(std::unique_ptr<game_logic> logic);
        game_logic* get_game_logic() { return game_logic_.get(); }

    private:
        // Алиасы для упрощения кода
        using time_point = std::chrono::high_resolution_clock::time_point;
        
        void main_loop();
        void update(float delta_time);
        void render();

        // Компоненты движка управляются через shared_ptr
        std::shared_ptr<window> window_;
        std::shared_ptr<vulkan_context> vulkan_context_;
        std::shared_ptr<renderer> renderer_;
        std::shared_ptr<camera> camera_;
        std::shared_ptr<world> world_;
        
        std::unique_ptr<game_logic> game_logic_;
        
        bool running_ = false;
        time_point last_frame_time_;
        
        // Подписка на события окна
        events::sub_id window_resize_subscription_ = 0;
    };
} 