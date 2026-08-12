#include <sculptor_version.h>

import std;

import vw.core;
import vw.gfx;
import vw.sculptor;

auto main() -> int {
    try {
        const auto title = std::format("Sculptor {}", vw::sculptor::version_string);
        vw::gfx::engine{1800, 1200, title}.run<vw::sculptor::app>();
    } catch (const std::exception& e) {
        vw::log::error("Ошибка выполнения: {}", e.what());
    }
    return 0;
}
