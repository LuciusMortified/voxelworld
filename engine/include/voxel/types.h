#pragma once

using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using int8  = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

namespace voxel {
    template<typename T>
    struct vec3 {
        T x, y, z;

        vec3() : x(0), y(0), z(0) {}
        vec3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}

        vec3 operator+(const vec3& other) const {
            return vec3(x + other.x, y + other.y, z + other.z);
        }
        vec3 operator-(const vec3& other) const {
            return vec3(x - other.x, y - other.y, z - other.z);
        }
        vec3 operator*(T scalar) const {
            return vec3(x * scalar, y * scalar, z * scalar);
        }
        bool operator==(const vec3& other) const {
            return x == other.x && y == other.y && z == other.z;
        }
        bool operator!=(const vec3& other) const {
            return !(*this == other);
        }
    };

    using vec3i = vec3<int>;
    using vec3f = vec3<float>;
    using dvec3 = vec3<double>;

    template<typename T>
    struct color {
        T r, g, b, a;

        color() : r(0), g(0), b(0), a(1) {}
        color(T r_, T g_, T b_, T a_ = T(1)) : r(r_), g(g_), b(b_), a(a_) {}
        color(T gray, T alpha = T(1)) : r(gray), g(gray), b(gray), a(alpha) {}

        // Конструктор из vec3 (RGB) с опциональным альфа-каналом
        color(const vec3<T>& rgb, T alpha = T(1)) : r(rgb.x), g(rgb.y), b(rgb.z), a(alpha) {}

        // Операторы для работы с цветами
        color operator+(const color& other) const {
            return color(r + other.r, g + other.g, b + other.b, a + other.a);
        }
        color operator-(const color& other) const {
            return color(r - other.r, g - other.g, b - other.b, a - other.a);
        }
        color operator*(T scalar) const {
            return color(r * scalar, g * scalar, b * scalar, a * scalar);
        }
        color operator*(const color& other) const {
            return color(r * other.r, g * other.g, b * other.b, a * other.a);
        }

        // Операторы сравнения
        bool operator==(const color& other) const {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }
        bool operator!=(const color& other) const {
            return !(*this == other);
        }

        // Методы для работы с цветом
        vec3<T> rgb() const { return vec3<T>(r, g, b); }
        vec3<T> hsv() const; // TODO: Реализовать конвертацию в HSV
        void set_hsv(T h, T s, T v); // TODO: Реализовать установку из HSV

        // Статические методы для создания предопределенных цветов
        static color black(T alpha = T(1)) { return color(T(0), T(0), T(0), alpha); }
        static color white(T alpha = T(1)) { return color(T(1), T(1), T(1), alpha); }
        static color red(T alpha = T(1)) { return color(T(1), T(0), T(0), alpha); }
        static color green(T alpha = T(1)) { return color(T(0), T(1), T(0), alpha); }
        static color blue(T alpha = T(1)) { return color(T(0), T(0), T(1), alpha); }
        static color yellow(T alpha = T(1)) { return color(T(1), T(1), T(0), alpha); }
        static color cyan(T alpha = T(1)) { return color(T(0), T(1), T(1), alpha); }
        static color magenta(T alpha = T(1)) { return color(T(1), T(0), T(1), alpha); }
        static color gray(T value, T alpha = T(1)) { return color(value, value, value, alpha); }
    };

    using colorf = color<float>;
    using colord = color<double>;


    template<typename T>
    struct mat4 {
        T data[16];

        mat4() {
            // Инициализация единичной матрицей
            for (int i = 0; i < 16; ++i) {
                data[i] = (i % 5 == 0) ? T(1) : T(0);
            }
        }

        mat4(const T* values) {
            for (int i = 0; i < 16; ++i) {
                data[i] = values[i];
            }
        }

        // Доступ к элементам через индексы
        T& operator()(int row, int col) {
            return data[row * 4 + col];
        }

        const T& operator()(int row, int col) const {
            return data[row * 4 + col];
        }

        // Доступ к элементам через линейный индекс
        T& operator[](int index) {
            return data[index];
        }

        const T& operator[](int index) const {
            return data[index];
        }

        // Получение указателя на данные
        T* ptr() { return data; }
        const T* ptr() const { return data; }

        // Операторы для матричных операций
        mat4 operator*(const mat4& other) const {
            mat4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result(i, j) = T(0);
                    for (int k = 0; k < 4; ++k) {
                        result(i, j) += (*this)(i, k) * other(k, j);
                    }
                }
            }
            return result;
        }

        // Оператор присваивания
        mat4& operator=(const mat4& other) {
            for (int i = 0; i < 16; ++i) {
                data[i] = other.data[i];
            }
            return *this;
        }

        // Сравнение
        bool operator==(const mat4& other) const {
            for (int i = 0; i < 16; ++i) {
                if (data[i] != other.data[i]) return false;
            }
            return true;
        }

        bool operator!=(const mat4& other) const {
            return !(*this == other);
        }
    };

    using mat4f = mat4<float>;
    using mat4d = mat4<double>;
    
    // Type aliases для идентификаторов объектов
    using object_id = uint32;
} 