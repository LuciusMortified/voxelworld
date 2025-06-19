#include <engine.h>
#include <voxel_model.h>

int main() {
    voxel::engine eng(1280, 720, "Voxel Example");

    voxel::voxel_model tree(3, 4, 1);
    tree.set_voxel(voxel::voxel(1, 0, 0, 0x00FF00));
    tree.set_voxel(voxel::voxel(1, 1, 0, 0x00FF00));
    tree.set_voxel(voxel::voxel(1, 2, 0, 0x00FF00));
    tree.set_voxel(voxel::voxel(1, 3, 0, 0x8B4513));

    eng.world().add_model(tree, voxel::ivec3(10, 5, 3));
    eng.run();
    return 0;
} 