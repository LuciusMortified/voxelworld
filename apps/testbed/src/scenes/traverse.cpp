module vw.testbed;

import std;
import vw.core;
import vw.gfx;

namespace vw::testbed {

auto parked_scene::drive_camera() -> void {
    auto& camera = stand().camera();
    camera.set_position({0.0f, eye_height(), 0.0f});
    camera.set_rotation(-10.0f, 0.0f);
}

auto spin_scene::drive_camera() -> void {
    auto& camera = stand().camera();
    camera.set_position({0.0f, eye_height(), 0.0f});
    camera.set_rotation(-10.0f, static_cast<float32>(frame_++) * degrees_per_frame);
}

auto advance_scene::drive_camera() -> void {
    // Только когда мир вокруг старта целый. Выход с холодного старта мерил
    // первые восемьсот кадров догона — а это не то же самое, что ходьба по уже
    // стоящему миру, и на вид похоже на сломанный загрузчик.
    if (!stand().is_bench_ready()) {
        return;
    }

    auto& camera = stand().camera();
    camera.set_position({
        static_cast<float32>(frame_++) * advance_per_frame,
        eye_height() + clearance,
        0.0f,
    });
    camera.set_rotation(-10.0f, 90.0f);
}

auto flythrough_scene::drive_camera() -> void {
    if (!stand().is_bench_ready()) {
        return;
    }

    const float32 angle = static_cast<float32>(frame_++) * degrees_per_frame;
    const float32 rad   = math::radians(angle);

    auto& camera = stand().camera();
    camera.set_position({
        std::sin(rad) * radius,
        eye_height() + clearance,
        std::cos(rad) * radius,
    });
    camera.set_rotation(-10.0f, angle + 90.0f);
}

}  // namespace vw::testbed
