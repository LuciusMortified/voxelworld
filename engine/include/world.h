#pragma once
#include <vector>

#include "types.h"
#include "model.h"

namespace voxel {
    class world {
    public:
        void add_model(const model& model, const ivec3& position);
        void remove_model(size_t index);
        
    private:
        struct placed_model {
            model model;
            ivec3 position;
        };
        std::vector<placed_model> models_;
    };
} 