#pragma once
#include <string_view>

#include "world.h"

namespace voxel {
    class engine {
    public:
        engine(int width, int height, std::string_view title);
        ~engine();

        void run();
        world& world();
    private:
        world world_;
    };
} 