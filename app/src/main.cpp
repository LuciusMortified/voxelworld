#include <engine/engine.h>
#include <engine/model.h>

int main() {
    voxel::engine eng(1280, 720, "Voxel Example");

    voxel::voxel_model tree(3, 4, 1); // ширина, высота, глубина
    tree.set_voxel({1, 0, 0, 0x00FF00}); // листва
    tree.set_voxel({1, 1, 0, 0x00FF00});
    tree.set_voxel({1, 2, 0, 0x00FF00});
    tree.set_voxel({1, 3, 0, 0x8B4513}); // ствол

    eng.world().add_model(tree, 10, 5, 3);
    eng.run();
    return 0;
} 