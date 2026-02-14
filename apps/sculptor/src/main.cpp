#include <sculptor_version.h>

#include "app/app.h"

int main() {
    try {
        auto title = std::format("Sculptor {}", vw::sculptor::version_string);
        vw::gfx::engine{1600, 1200, title}.run<vw::sculptor::app>();
    } catch (const std::exception& e) {
        vw::log::error("Ошибка выполнения: {}", e.what());
    }
    return 0;
}