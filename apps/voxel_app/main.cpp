#include <iostream>

#include "engine.h"
#include "voxel.h"

int main() {
    try {
        std::cout << "Запуск Voxel Engine..." << std::endl;
        
        // Создание и запуск движка
        voxel::engine engine(1280, 720, "Voxel World");
        
        // Создание простой модели дерева
        voxel::model tree(3, 4, 1);
        tree.set_voxel(1, 0, 0, voxel::colors::GREEN);
        tree.set_voxel(1, 1, 0, voxel::colors::GREEN);
        tree.set_voxel(1, 2, 0, voxel::colors::GREEN);
        tree.set_voxel(1, 3, 0, voxel::colors::BROWN);
        
        // Добавление модели в мир
        engine.get_world().add_model(tree, {10, 5, 3});
        
        // Запуск главного цикла
        engine.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 