#pragma once
#include "types.h"

namespace voxel {
    struct voxel {
        uint32 color;

        voxel() : color(0) {}
        voxel(uint32 c) : color(c) {}
    };

    // 255-цветная палитра - основные цвета
    namespace colors {
        // Прозрачный (пустой воксель)
        constexpr uint32 TRANSPARENT = 0x00000000;
        
        // Основные цвета (RGB)
        constexpr uint32 BLACK       = 0x000000FF;
        constexpr uint32 WHITE       = 0xFFFFFFFF;
        constexpr uint32 RED         = 0xFF0000FF;
        constexpr uint32 GREEN       = 0x00FF00FF;
        constexpr uint32 BLUE        = 0x0000FFFF;
        constexpr uint32 YELLOW      = 0xFFFF00FF;
        constexpr uint32 CYAN        = 0x00FFFFFF;
        constexpr uint32 MAGENTA     = 0xFF00FFFF;
        
        // Оттенки серого
        constexpr uint32 GRAY        = 0x808080FF;
        constexpr uint32 LIGHT_GRAY  = 0xC0C0C0FF;
        constexpr uint32 DARK_GRAY   = 0x404040FF;
        
        // Оттенки красного
        constexpr uint32 DARK_RED    = 0x800000FF;
        constexpr uint32 LIGHT_RED   = 0xFF8080FF;
        constexpr uint32 PINK        = 0xFFC0CBFF;
        constexpr uint32 CRIMSON     = 0xDC143CFF;
        constexpr uint32 MAROON      = 0x800000FF;
        
        // Оттенки зеленого
        constexpr uint32 DARK_GREEN  = 0x008000FF;
        constexpr uint32 LIGHT_GREEN = 0x90EE90FF;
        constexpr uint32 LIME        = 0x00FF00FF;
        constexpr uint32 FOREST_GREEN = 0x228B22FF;
        constexpr uint32 OLIVE       = 0x808000FF;
        
        // Оттенки синего
        constexpr uint32 DARK_BLUE   = 0x000080FF;
        constexpr uint32 LIGHT_BLUE  = 0xADD8E6FF;
        constexpr uint32 NAVY        = 0x000080FF;
        constexpr uint32 SKY_BLUE    = 0x87CEEBFF;
        constexpr uint32 ROYAL_BLUE  = 0x4169E1FF;
        
        // Оттенки желтого/оранжевого
        constexpr uint32 DARK_YELLOW = 0x808000FF;
        constexpr uint32 LIGHT_YELLOW = 0xFFFFE0FF;
        constexpr uint32 GOLD        = 0xFFD700FF;
        constexpr uint32 ORANGE      = 0xFFA500FF;
        constexpr uint32 DARK_ORANGE = 0xFF8C00FF;
        
        // Оттенки коричневого
        constexpr uint32 BROWN       = 0xA52A2AFF;
        constexpr uint32 LIGHT_BROWN = 0xD2691EFF;
        constexpr uint32 DARK_BROWN  = 0x654321FF;
        constexpr uint32 TAN         = 0xD2B48CFF;
        constexpr uint32 CHOCOLATE   = 0xD2691EFF;
        
        // Оттенки фиолетового
        constexpr uint32 PURPLE      = 0x800080FF;
        constexpr uint32 LIGHT_PURPLE = 0xE6E6FAFF;
        constexpr uint32 DARK_PURPLE = 0x483D8BFF;
        constexpr uint32 VIOLET      = 0xEE82EEFF;
        constexpr uint32 INDIGO      = 0x4B0082FF;
        
        // Металлические цвета
        constexpr uint32 SILVER      = 0xC0C0C0FF;
        constexpr uint32 GOLD_METAL  = 0xFFD700FF;
        constexpr uint32 BRONZE      = 0xCD7F32FF;
        constexpr uint32 COPPER      = 0xB87333FF;
        constexpr uint32 IRON        = 0x696969FF;
        
        // Природные цвета
        constexpr uint32 GRASS       = 0x7CFC00FF;
        constexpr uint32 DIRT        = 0x8B4513FF;
        constexpr uint32 SAND        = 0xF4A460FF;
        constexpr uint32 STONE       = 0x808080FF;
        constexpr uint32 WATER       = 0x4682B4FF;
        constexpr uint32 ICE         = 0xF0F8FFFF;
        constexpr uint32 FIRE        = 0xFF4500FF;
        constexpr uint32 LAVA        = 0xFF4500FF;
        constexpr uint32 WOOD        = 0x8B4513FF;
        constexpr uint32 LEAVES      = 0x228B22FF;
        
        // Пастельные цвета
        constexpr uint32 PASTEL_PINK = 0xFFB6C1FF;
        constexpr uint32 PASTEL_BLUE = 0xB0E0E6FF;
        constexpr uint32 PASTEL_GREEN = 0x98FB98FF;
        constexpr uint32 PASTEL_YELLOW = 0xF0E68CFF;
        constexpr uint32 PASTEL_PURPLE = 0xDDA0DDFF;
        constexpr uint32 PASTEL_ORANGE = 0xFFB347FF;
        
        // Неоновые цвета
        constexpr uint32 NEON_PINK   = 0xFF1493FF;
        constexpr uint32 NEON_BLUE   = 0x00BFFFFF;
        constexpr uint32 NEON_GREEN  = 0x39FF14FF;
        constexpr uint32 NEON_YELLOW = 0xFFFF00FF;
        constexpr uint32 NEON_ORANGE = 0xFF8C00FF;
        constexpr uint32 NEON_PURPLE = 0x9400D3FF;
    }
}