#pragma once
#include <vector>

#include <vw/types.h>
#include <vw/gfx/voxel.h>

namespace vw::gfx {
    class model {
    public:
        model(int width, int height, int depth);

        void set_voxel(int x, int y, int z, const voxel& voxel);
        void set_voxel(int x, int y, int z, color color) {
            set_voxel(x, y, z, voxel{color});
        }

        [[nodiscard]]
        voxel get_voxel(int x, int y, int z) const;

        [[nodiscard]]
        bool is_empty(int x, int y, int z) const;

        [[nodiscard]]
        int width() const { return width_; }

        [[nodiscard]]
        int height() const { return height_; }

        [[nodiscard]]
        int depth() const { return depth_; }

        void clear();
        void fill(const voxel& voxel);
        
    private:
        int width_{0}, height_{0}, depth_{0};

        std::vector<voxel> voxels_;

        [[nodiscard]]
        int index(int x, int y, int z) const;
    };
} // namespace vw::gfx