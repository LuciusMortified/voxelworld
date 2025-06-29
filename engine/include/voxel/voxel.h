#pragma once

#include <voxel/types.h>

namespace voxel {
    struct voxel {
        uint32 color;

        constexpr voxel() : color(0) {}
        constexpr voxel(uint32 c) : color(c) {}
        
        // Проверка на прозрачность
        constexpr bool is_empty() const { return color == 0; }
        constexpr bool is_transparent() const { return is_empty(); }
    };

    // 255-цветная палитра - основные цвета
    // Прозрачный (пустой воксель)
    constexpr voxel TRANSPARENT = voxel(0x00000000);
    
    // Основные цвета (RGB)
    constexpr voxel BLACK       = voxel(0x000000FF);
    constexpr voxel WHITE       = voxel(0xFFFFFFFF);
    constexpr voxel RED         = voxel(0xFF0000FF);
    constexpr voxel GREEN       = voxel(0x00FF00FF);
    constexpr voxel BLUE        = voxel(0x0000FFFF);
    constexpr voxel YELLOW      = voxel(0xFFFF00FF);
    constexpr voxel CYAN        = voxel(0x00FFFFFF);
    constexpr voxel MAGENTA     = voxel(0xFF00FFFF);
    
    // Оттенки серого
    constexpr voxel GRAY        = voxel(0x808080FF);
    constexpr voxel LIGHT_GRAY  = voxel(0xC0C0C0FF);
    constexpr voxel DARK_GRAY   = voxel(0x404040FF);
    
    // Оттенки красного
    constexpr voxel DARK_RED    = voxel(0x800000FF);
    constexpr voxel LIGHT_RED   = voxel(0xFF8080FF);
    constexpr voxel PINK        = voxel(0xFFC0CBFF);
    constexpr voxel CRIMSON     = voxel(0xDC143CFF);
    constexpr voxel MAROON      = voxel(0x800000FF);
    
    // Оттенки зеленого
    constexpr voxel DARK_GREEN  = voxel(0x008000FF);
    constexpr voxel LIGHT_GREEN = voxel(0x90EE90FF);
    constexpr voxel LIME        = voxel(0x00FF00FF);
    constexpr voxel FOREST_GREEN = voxel(0x228B22FF);
    constexpr voxel OLIVE       = voxel(0x808000FF);
    
    // Оттенки синего
    constexpr voxel DARK_BLUE   = voxel(0x000080FF);
    constexpr voxel LIGHT_BLUE  = voxel(0xADD8E6FF);
    constexpr voxel NAVY        = voxel(0x000080FF);
    constexpr voxel SKY_BLUE    = voxel(0x87CEEBFF);
    constexpr voxel ROYAL_BLUE  = voxel(0x4169E1FF);
    
    // Оттенки желтого/оранжевого
    constexpr voxel DARK_YELLOW = voxel(0x808000FF);
    constexpr voxel LIGHT_YELLOW = voxel(0xFFFFE0FF);
    constexpr voxel GOLD        = voxel(0xFFD700FF);
    constexpr voxel ORANGE      = voxel(0xFFA500FF);
    constexpr voxel DARK_ORANGE = voxel(0xFF8C00FF);
    
    // Оттенки коричневого
    constexpr voxel BROWN       = voxel(0xA52A2AFF);
    constexpr voxel LIGHT_BROWN = voxel(0xD2691EFF);
    constexpr voxel DARK_BROWN  = voxel(0x654321FF);
    constexpr voxel TAN         = voxel(0xD2B48CFF);
    constexpr voxel CHOCOLATE   = voxel(0xD2691EFF);
    
    // Оттенки фиолетового
    constexpr voxel PURPLE      = voxel(0x800080FF);
    constexpr voxel LIGHT_PURPLE = voxel(0xE6E6FAFF);
    constexpr voxel DARK_PURPLE = voxel(0x483D8BFF);
    constexpr voxel VIOLET      = voxel(0xEE82EEFF);
    constexpr voxel INDIGO      = voxel(0x4B0082FF);
    
    // Металлические цвета
    constexpr voxel SILVER      = voxel(0xC0C0C0FF);
    constexpr voxel GOLD_METAL  = voxel(0xFFD700FF);
    constexpr voxel BRONZE      = voxel(0xCD7F32FF);
    constexpr voxel COPPER      = voxel(0xB87333FF);
    constexpr voxel IRON        = voxel(0x696969FF);
    
    // Природные цвета
    constexpr voxel GRASS       = voxel(0x7CFC00FF);
    constexpr voxel DIRT        = voxel(0x8B4513FF);
    constexpr voxel SAND        = voxel(0xF4A460FF);
    constexpr voxel STONE       = voxel(0x808080FF);
    constexpr voxel WATER       = voxel(0x4682B4FF);
    constexpr voxel ICE         = voxel(0xF0F8FFFF);
    constexpr voxel FIRE        = voxel(0xFF4500FF);
    constexpr voxel LAVA        = voxel(0xFF4500FF);
    constexpr voxel WOOD        = voxel(0x8B4513FF);
    constexpr voxel LEAVES      = voxel(0x228B22FF);
    
    // Пастельные цвета
    constexpr voxel PASTEL_PINK = voxel(0xFFB6C1FF);
    constexpr voxel PASTEL_BLUE = voxel(0xB0E0E6FF);
    constexpr voxel PASTEL_GREEN = voxel(0x98FB98FF);
    constexpr voxel PASTEL_YELLOW = voxel(0xF0E68CFF);
    constexpr voxel PASTEL_PURPLE = voxel(0xDDA0DDFF);
    constexpr voxel PASTEL_ORANGE = voxel(0xFFB347FF);
    
    // Неоновые цвета
    constexpr voxel NEON_PINK   = voxel(0xFF1493FF);
    constexpr voxel NEON_BLUE   = voxel(0x00BFFFFF);
    constexpr voxel NEON_GREEN  = voxel(0x39FF14FF);
    constexpr voxel NEON_YELLOW = voxel(0xFFFF00FF);
    constexpr voxel NEON_ORANGE = voxel(0xFF8C00FF);
    constexpr voxel NEON_PURPLE = voxel(0x9400D3FF);
}