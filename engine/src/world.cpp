#include "world.h"

namespace voxel {
    void world::add_model(const model& model, const ivec3& position) {
        models_.push_back(placed_model{model, position});
    }
    
    void world::remove_model(size_t index) {
        if (index < models_.size())
            models_.erase(models_.begin() + index);
    }
} 