#pragma once

#ifndef VW_MAT4_H
#define VW_MAT4_H

#include "vw/types.h"

namespace vw {
template <typename T>
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
    T* ptr() {
        return data;
    }
    const T* ptr() const {
        return data;
    }

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
            if (data[i] != other.data[i])
                return false;
        }
        return true;
    }

    bool operator!=(const mat4& other) const {
        return !(*this == other);
    }
};

using mat4f = mat4<float32>;
using mat4d = mat4<float64>;
}  // namespace vw

#endif  // VW_MAT4_H
