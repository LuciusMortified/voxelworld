#pragma once
#include <string_view>

#include "voxel_world.h"

namespace voxel {
    class engine {
    public:
        engine(int width, int height, std::string_view title);
        ~engine();

        void run();
        voxel_world& world();
    private:
        voxel_world world_;
    };
} 