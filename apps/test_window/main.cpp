#include <chrono>
#include <iostream>
#include <thread>

#include <../../engine/include/vw/gfx/window/input.h>
#include <../../engine/include/vw/gfx/window/window.h>

int main() {
    try {
        // Создание окна
        vw::gfx::window window(1280, 720, "Voxel World - Test Window");

        // Главный цикл
        while (!window.should_close()) {
            // Обработка событий
            window.poll_events();

            // Проверка нажатий клавиш (используем алиасы)
            if (window.is_key_pressed(vw::gfx::keyboard::key::W)) {
                std::cout << "Нажата клавиша W" << std::endl;
            }
            if (window.is_key_pressed(vw::gfx::keyboard::key::A)) {
                std::cout << "Нажата клавиша A" << std::endl;
            }
            if (window.is_key_pressed(vw::gfx::keyboard::key::S)) {
                std::cout << "Нажата клавиша S" << std::endl;
            }
            if (window.is_key_pressed(vw::gfx::keyboard::key::D)) {
                std::cout << "Нажата клавиша D" << std::endl;
            }

            // Проверка кнопок мыши
            if (window.is_mouse_button_pressed(vw::gfx::mouse::button::LEFT)) {
                std::cout << "Нажата левая кнопка мыши" << std::endl;
            }
            if (window.is_mouse_button_pressed(vw::gfx::mouse::button::RIGHT)) {
                std::cout << "Нажата правая кнопка мыши" << std::endl;
            }

            // Проверка позиции мыши
            vw::vec2d mouse_pos = window.get_cursor_pos();
            std::cout << "Позиция мыши: (" << mouse_pos.x << ", " << mouse_pos.y << ")"
                      << std::endl;

            // Небольшая задержка для снижения нагрузки на CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
        }
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}