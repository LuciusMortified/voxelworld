#include "engine.h"

namespace voxel {
    engine::engine(int width, int height, std::string_view title) {
        // TODO: инициализация Vulkan, окна и т.д.
    }
    engine::~engine() {
        // TODO: очистка ресурсов
    }
    void engine::run() {
        // TODO: главный цикл
    }
    voxel_world& engine::world() {
        return world_;
    }
} 