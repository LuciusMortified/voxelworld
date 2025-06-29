#pragma once
#include <vector>

#include <voxel/types.h>
#include <voxel/voxel.h>

namespace voxel {
    class model {
    public:
        model(int width, int height, int depth);
        
        // Методы для работы с voxel объектами
        void set_voxel(int x, int y, int z, const voxel& voxel);
        voxel get_voxel(int x, int y, int z) const;
        
        // Проверка существования воксела
        bool has_voxel(int x, int y, int z) const;
        bool is_empty(int x, int y, int z) const;
        
        // Размеры модели
        int width() const { return width_; }
        int height() const { return height_; }
        int depth() const { return depth_; }
        
        // Очистка модели
        void clear();
        void fill(const voxel& voxel);
        
    private:
        int width_, height_, depth_;
        std::vector<voxel> voxels_;
        int index(int x, int y, int z) const;
    };
} 