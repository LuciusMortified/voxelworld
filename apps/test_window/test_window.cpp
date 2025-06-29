#include <iostream>
#include <chrono>
#include <thread>

#include <voxel/window.h>
#include <voxel/input.h>

int main() {
    try {
        std::cout << "Создание окна..." << std::endl;
        
        // Создание окна
        voxel::window window(1280, 720, "Voxel Engine - Window Test");
        
        std::cout << "Окно создано успешно!" << std::endl;
        std::cout << "Размер окна: " << window.get_width() << "x" << window.get_height() << std::endl;
        std::cout << "Нажмите ESC для выхода" << std::endl;
        
        // Главный цикл
        while (!window.should_close()) {
            // Обработка событий
            window.poll_events();
            
            // Проверка нажатий клавиш (используем алиасы)
            if (window.is_key_pressed(voxel::input::key::W)) {
                std::cout << "Нажата клавиша W" << std::endl;
            }
            if (window.is_key_pressed(voxel::input::key::A)) {
                std::cout << "Нажата клавиша A" << std::endl;
            }
            if (window.is_key_pressed(voxel::input::key::S)) {
                std::cout << "Нажата клавиша S" << std::endl;
            }
            if (window.is_key_pressed(voxel::input::key::D)) {
                std::cout << "Нажата клавиша D" << std::endl;
            }
            
            // Проверка кнопок мыши
            if (window.is_mouse_button_pressed(voxel::input::mouse_button::LEFT)) {
                std::cout << "Нажата левая кнопка мыши" << std::endl;
            }
            if (window.is_mouse_button_pressed(voxel::input::mouse_button::RIGHT)) {
                std::cout << "Нажата правая кнопка мыши" << std::endl;
            }
            
            // Проверка позиции мыши
            double mouse_x, mouse_y;
            window.get_cursor_pos(&mouse_x, &mouse_y);
            
            // Небольшая задержка для снижения нагрузки на CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        }
        
        std::cout << "Окно закрыто" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 