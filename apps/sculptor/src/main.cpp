#include <sculptor_version.h>

#include "app/app.h"

int main() {
    try {
        auto title = std::format("Voxel World - Sculptor {}", vw::sculptor::version_string);
        vw::gfx::engine{1280, 720, title}.run<vw::sculptor::app>();
    } catch (const std::exception& e) {
        vw::log::error("Ошибка выполнения: {}", e.what());
    }
    return 0;
}