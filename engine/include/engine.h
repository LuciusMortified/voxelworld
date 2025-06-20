#pragma once
#include <string_view>
#include <memory>

#include "world.h"

namespace voxel {
    class window;

    class engine {
    public:
        engine(int width, int height, std::string_view title);
        ~engine();

        // Запретить копирование
        engine(const engine&) = delete;
        engine& operator=(const engine&) = delete;

        void run();
        
        world& get_world() { return world_; }

    private:
        void initialize();
        void cleanup();
        void update(float delta_time);
        void render();

        world world_;
        std::unique_ptr<window> window_;
    };
} 