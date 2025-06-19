#pragma once
#include <vector>

#include "model.h"

namespace voxel {
    class world {
    public:
        void add_model(const model& model, int x, int y, int z);
        void remove_model(size_t index);
        
    private:
        struct placed_model {
            model model;
            int x, y, z;
        };
        std::vector<placed_model> models_;
    };
} 