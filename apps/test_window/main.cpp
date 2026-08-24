import std;

import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import vw.gfx;

using namespace vw;

auto main() -> int {
    try {
        // Создание окна
        plat::window window(1280, 720, "Voxel World - Test Window");

        // Главный цикл
        while (!window.should_close()) {
            // Обработка событий
            window.poll_events();

            // Проверка нажатий клавиш (используем алиасы)
            if (window.is_key_pressed(plat::keyboard::keys::W)) {
                log::info("Нажата клавиша W");
            }
            if (window.is_key_pressed(plat::keyboard::keys::A)) {
                log::info("Нажата клавиша A");
            }
            if (window.is_key_pressed(plat::keyboard::keys::S)) {
                log::info("Нажата клавиша S");
            }
            if (window.is_key_pressed(plat::keyboard::keys::D)) {
                log::info("Нажата клавиша D");
            }

            // Проверка кнопок мыши
            if (window.is_mouse_button_pressed(plat::mouse::buttons::LEFT)) {
                vec2d mouse_pos = window.get_cursor_pos();
                log::info("Нажата левая кнопка мыши ({}, {})", mouse_pos.x, mouse_pos.y);
            }
            if (window.is_mouse_button_pressed(plat::mouse::buttons::RIGHT)) {
                vec2d mouse_pos = window.get_cursor_pos();
                log::info("Нажата правая кнопка мыши ({}, {})", mouse_pos.x, mouse_pos.y);
            }

            // Небольшая задержка для снижения нагрузки на CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
        }
    } catch (const std::exception& e) {
        log::error("Ошибка: {}", e.what());
        return 1;
    }

    return 0;
}