#include "world.h"

namespace voxel {
    void voxel_world::add_model(const voxel_model& model, int x, int y, int z) {
        models_.push_back(placed_model{model, x, y, z});
    }
    void voxel_world::remove_model(size_t index) {
        if (index < models_.size())
            models_.erase(models_.begin() + index);
    }
} 