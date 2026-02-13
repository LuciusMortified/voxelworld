#pragma once

#ifndef VW_CORE_MAT4_INL_H
#define VW_CORE_MAT4_INL_H

#include <cstring>

#include "vw/core/mat4.h"
#include "vw/core/vec3.h"
#include "vw/core/vec4.h"

namespace vw {

template <typename T>
mat4<T>::mat4(
    const T* values
) {
    memcpy(data_.data(), values, size_ * sizeof(T));
}

template <typename T>
auto mat4<T>::operator[](
    int row, int col
) -> T& {
    return data_[(col * 4) + row];
}

template <typename T>
auto mat4<T>::operator[](
    int row, int col
) const -> const T& {
    return data_[(col * 4) + row];
}

template <typename T>
auto mat4<T>::operator[](
    int index
) -> T& {
    return data_[index];
}

template <typename T>
auto mat4<T>::operator[](
    int index
) const -> const T& {
    return data_[index];
}

template <typename T>
auto mat4<T>::cptr() -> T* {
    return data_.data();
}

template <typename T>
auto mat4<T>::cptr() const -> const T* {
    return data_.data();
}

template <typename T>
auto mat4<T>::operator*(
    const mat4& other
) const -> mat4 {
    mat4 result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i, j] = T(0);
            for (int k = 0; k < 4; ++k) {
                result[i, j] += (*this)[i, k] * other[k, j];
            }
        }
    }
    return result;
}

template <typename T>
auto mat4<T>::operator*(
    const vec4<T>& v
) const -> vec4<T> {
    auto& m = *this;
    vec4<T> r;

    r.x = m[0, 0] * v.x + m[0, 1] * v.y + m[0, 2] * v.z + m[0, 3] * v.w;
    r.y = m[1, 0] * v.x + m[1, 1] * v.y + m[1, 2] * v.z + m[1, 3] * v.w;
    r.z = m[2, 0] * v.x + m[2, 1] * v.y + m[2, 2] * v.z + m[2, 3] * v.w;
    r.w = m[3, 0] * v.x + m[3, 1] * v.y + m[3, 2] * v.z + m[3, 3] * v.w;

    return r;
}

template <typename T>
auto mat4<T>::operator*(
    const vec3<T>& vec
) const -> vec3<T> {
    vec4<T> h{vec.x, vec.y, vec.z, T(1)};
    vec4<T> r = (*this) * h;

    if (r.w != T(0) && r.w != T(1)) {
        r.x /= r.w;
        r.y /= r.w;
        r.z /= r.w;
    }

    return {r.x, r.y, r.z};
}

template <typename T>
auto mat4<T>::operator==(
    const mat4& other
) const -> bool {
    for (int i = 0; i < size_; ++i) {
        if (data_[i] != other.data_[i]) {
            return false;
        }
    }
    return true;
}

template <typename T>
auto mat4<T>::operator!=(
    const mat4& other
) const -> bool {
    return !(*this == other);
}

template <typename T>
auto mat4<T>::operator+(const mat4& other) const -> mat4 {
    mat4 result;
    for (int i = 0; i < size_; ++i) {
        result.data_[i] = data_[i] + other.data_[i];
    }
    return result;
}

template <typename T>
auto mat4<T>::operator-(const mat4& other) const -> mat4 {
    mat4 result;
    for (int i = 0; i < size_; ++i) {
        result.data_[i] = data_[i] - other.data_[i];
    }
    return result;
}

template <typename T>
auto mat4<T>::operator-() const -> mat4 {
    mat4 result;
    for (int i = 0; i < size_; ++i) {
        result.data_[i] = -data_[i];
    }
    return result;
}

template <typename T>
auto mat4<T>::operator*(T scalar) const -> mat4 {
    mat4 result;
    for (int i = 0; i < size_; ++i) {
        result.data_[i] = data_[i] * scalar;
    }
    return result;
}

template <typename T>
auto mat4<T>::operator/(T scalar) const -> mat4 {
    mat4 result;
    for (int i = 0; i < size_; ++i) {
        result.data_[i] = data_[i] / scalar;
    }
    return result;
}

template <typename T>
auto mat4<T>::operator+=(const mat4& other) -> mat4& {
    for (int i = 0; i < size_; ++i) {
        data_[i] += other.data_[i];
    }
    return *this;
}

template <typename T>
auto mat4<T>::operator-=(const mat4& other) -> mat4& {
    for (int i = 0; i < size_; ++i) {
        data_[i] -= other.data_[i];
    }
    return *this;
}

template <typename T>
auto mat4<T>::operator*=(const mat4& other) -> mat4& {
    *this = *this * other;
    return *this;
}

template <typename T>
auto mat4<T>::operator*=(T scalar) -> mat4& {
    for (int i = 0; i < size_; ++i) {
        data_[i] *= scalar;
    }
    return *this;
}

template <typename T>
auto mat4<T>::operator/=(T scalar) -> mat4& {
    for (int i = 0; i < size_; ++i) {
        data_[i] /= scalar;
    }
    return *this;
}

template <typename T>
auto operator*(T scalar, const mat4<T>& m) -> mat4<T> {
    return m * scalar;
}

}  // namespace vw

#endif  // VW_CORE_MAT4_INL_H
