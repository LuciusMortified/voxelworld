#pragma once

#ifndef VW_MAT4_H
#define VW_MAT4_H

#include <array>

#include "vw/core/types.h"

namespace vw {
template <typename T>
struct vec3;
template <typename T>
struct vec4;

template <typename T>
struct mat4 {
private:
    constexpr static size_t size_ = 16;
    std::array<T, size_> data_{};

public:
    mat4() = default;

    explicit mat4(const T* values);

    mat4(const mat4& other)                    = default;
    auto operator=(const mat4& other) -> mat4& = default;
    mat4(mat4&& other)                         = default;
    auto operator=(mat4&& other) -> mat4&      = default;

    auto operator[](int row, int col) -> T&;
    auto operator[](int row, int col) const -> const T&;
    auto operator[](int index) -> T&;
    auto operator[](int index) const -> const T&;

    auto cptr() -> T*;
    auto cptr() const -> const T*;

    auto operator*(const mat4& other) const -> mat4;
    auto operator*(const vec4<T>& v) const -> vec4<T>;
    auto operator*(const vec3<T>& vec) const -> vec3<T>;

    auto operator==(const mat4& other) const -> bool;
    auto operator!=(const mat4& other) const -> bool;
};

using mat4f = mat4<float32>;
using mat4d = mat4<float64>;
}  // namespace vw

#include "vw/core/mat4.inl.h"

#endif  // VW_MAT4_H
