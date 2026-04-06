#include <vw/core.h>
#include <vw/gfx.h>

#include "app/arena_app.h"

auto main() -> int {
    try {
        vw::log::logger::get().add_file_sink("arena.log");
        std::make_unique<vw::gfx::engine<>>(1280, 720, "Voxel World - Arena")
            ->run<vw::arena::arena_app>();
    } catch (const std::exception& e) {
        vw::log::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
