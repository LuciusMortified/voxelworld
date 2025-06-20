#pragma once

namespace voxel {
namespace input {

// Клавиши клавиатуры
enum class key {
    // Буквы
    A = 65, B = 66, C = 67, D = 68, E = 69, F = 70, G = 71, H = 72, I = 73, J = 74,
    K = 75, L = 76, M = 77, N = 78, O = 79, P = 80, Q = 81, R = 82, S = 83, T = 84,
    U = 85, V = 86, W = 87, X = 88, Y = 89, Z = 90,
    
    // Цифры
    KEY_0 = 48, KEY_1 = 49, KEY_2 = 50, KEY_3 = 51, KEY_4 = 52,
    KEY_5 = 53, KEY_6 = 54, KEY_7 = 55, KEY_8 = 56, KEY_9 = 57,
    
    // Функциональные клавиши
    F1 = 290, F2 = 291, F3 = 292, F4 = 293, F5 = 294, F6 = 295,
    F7 = 296, F8 = 297, F9 = 298, F10 = 299, F11 = 300, F12 = 301,
    
    // Специальные клавиши
    ESCAPE = 256, ENTER = 257, TAB = 258, SPACE = 32, BACKSPACE = 259,
    DELETE = 261, INSERT = 260, HOME = 268, END = 269, PAGE_UP = 266, PAGE_DOWN = 267,
    
    // Стрелки
    LEFT = 263, RIGHT = 262, UP = 265, DOWN = 264,
    
    // Модификаторы
    LEFT_SHIFT = 340, RIGHT_SHIFT = 344,
    LEFT_CONTROL = 341, RIGHT_CONTROL = 345,
    LEFT_ALT = 342, RIGHT_ALT = 346,
    LEFT_SUPER = 343, RIGHT_SUPER = 347, // Windows/Command
    
    // Дополнительные клавиши
    GRAVE_ACCENT = 96, // `
    MINUS = 45, EQUAL = 61,
    LEFT_BRACKET = 91, RIGHT_BRACKET = 93, // [ ]
    BACKSLASH = 92, SEMICOLON = 59, APOSTROPHE = 39,
    COMMA = 44, PERIOD = 46, SLASH = 47,
    
    // Numpad
    NUM_0 = 320, NUM_1 = 321, NUM_2 = 322, NUM_3 = 323, NUM_4 = 324,
    NUM_5 = 325, NUM_6 = 326, NUM_7 = 327, NUM_8 = 328, NUM_9 = 329,
    NUM_DECIMAL = 330, NUM_DIVIDE = 331, NUM_MULTIPLY = 332,
    NUM_SUBTRACT = 333, NUM_ADD = 334, NUM_ENTER = 335, NUM_EQUAL = 336
};

// Кнопки мыши
enum class mouse_button {
    LEFT = 0,
    RIGHT = 1,
    MIDDLE = 2,
    BUTTON_4 = 3,
    BUTTON_5 = 4,
    BUTTON_6 = 5,
    BUTTON_7 = 6,
    BUTTON_8 = 7
};

// Режимы курсора
enum class cursor_mode {
    NORMAL = 0x00034001,      // Обычный курсор
    HIDDEN = 0x00034002,      // Скрытый курсор
    DISABLED = 0x00034003     // Заблокированный курсор (для FPS камеры)
};

// Режимы ввода
enum class input_mode {
    CURSOR = 0x00033001,      // Режим курсора
    STICKY_KEYS = 0x00033002, // Залипающие клавиши
    STICKY_MOUSE_BUTTONS = 0x00033003, // Залипающие кнопки мыши
    LOCK_KEY_MODS = 0x00033004, // Блокировка модификаторов
    RAW_MOUSE_MOTION = 0x00033005 // Сырое движение мыши
};

// Состояния нажатия
enum class action {
    RELEASE = 0,
    PRESS = 1,
    REPEAT = 2
};

// Модификаторы
enum class mod {
    SHIFT = 0x0001,
    CONTROL = 0x0002,
    ALT = 0x0004,
    SUPER = 0x0008,
    CAPS_LOCK = 0x0010,
    NUM_LOCK = 0x0020
};

} // namespace input
} // namespace voxel 