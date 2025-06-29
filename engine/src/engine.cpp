#include <iostream>
#include <stdexcept>
#include <chrono>

#include <voxel/engine.h>
#include <voxel/window.h>
#include <voxel/vulkan_context.h>
#include <voxel/renderer.h>
#include <voxel/camera.h>
#include <voxel/game_logic.h>
#include <voxel/input.h>
#include <voxel/world.h>

namespace voxel {

engine::engine(int width, int height, std::string_view title) {
    // Создаем компоненты через shared_ptr
    window_ = std::make_shared<window>(width, height, title);
    vulkan_context_ = std::make_shared<vulkan_context>(window_);
    renderer_ = std::make_shared<renderer>(vulkan_context_, window_);
    camera_ = std::make_shared<camera>(45.0f, static_cast<float>(width) / height);
    world_ = std::make_shared<world>(vulkan_context_);
    
    // Создаем пустую game_logic по умолчанию
    game_logic_ = std::make_unique<game_logic>();
    
    // Подписка на изменение размера окна
    window_resize_subscription_ = window_->on<events::window_resize>(
        [this](events::window_resize& event) -> bool {
            if (event.width > 0 && event.height > 0) {
                float aspect = static_cast<float>(event.width) / event.height;
                camera_->set_aspect_ratio(aspect);
            }
            return false; // Не обрабатываем событие полностью
        }
    );
}

engine::~engine() {
    shutdown();
}

void engine::run(std::unique_ptr<game_logic> logic) {
    if (logic) {
        game_logic_ = std::move(logic);
    }
    
    if (game_logic_) {
        // Передаем shared_ptr на engine используя shared_from_this
        game_logic_->initialize(shared_from_this());
    }
    
    std::cout << "Запуск главного цикла..." << std::endl;
    main_loop();
    std::cout << "Главный цикл завершен" << std::endl;
}

void engine::shutdown() {
    std::cout << "Завершение работы Voxel Engine..." << std::endl;
    
    // Останавливаем главный цикл
    running_ = false;
    
    // Очистка игровой логики
    game_logic_->cleanup();
    
    // Ожидание завершения операций GPU
    renderer_->wait_idle();
    
    std::cout << "Ресурсы очищены" << std::endl;
}

void engine::main_loop() {
    running_ = true;
    last_frame_time_ = std::chrono::high_resolution_clock::now();
    
    while (running_ && !window_->should_close()) {
        // Вычисление delta time
        time_point current_time = std::chrono::high_resolution_clock::now();
        float delta_time = std::chrono::duration<float>(current_time - last_frame_time_).count();
        last_frame_time_ = current_time;
        
        // Ограничение delta time для стабильности
        if (delta_time > 0.1f) delta_time = 0.1f;
        
        // Обработка событий окна (GLFW callbacks автоматически вызывают event_dispatcher)
        window_->poll_events();
        
        // Проверка необходимости выхода
        if (!running_) {
            break;
        }
        
        // Обновление логики
        update(delta_time);
        
        // Рендеринг
        render();
    }
}

void engine::update(float delta_time) {
    // Обновление игровой логики
    game_logic_->update(delta_time);
    
    // Удаляем неэффективный код получения размера окна каждый кадр
    // Теперь aspect ratio обновляется только при изменении размера окна
}

void engine::render() {
    try {
        // Предварительный рендеринг игровой логики
        game_logic_->render();
        
        // Основной рендеринг движка
        renderer_->begin_frame();
        
        // Рендеринг мира
        renderer_->render_world(world_, camera_);
        
        renderer_->end_frame();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка рендеринга: " << e.what() << std::endl;
    }
}

void engine::set_game_logic(std::unique_ptr<game_logic> logic) {
    game_logic_ = std::move(logic);
}

} // namespace voxel 