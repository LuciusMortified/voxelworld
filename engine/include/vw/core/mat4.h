#pragma once

#ifndef VW_MAT4_H
#define VW_MAT4_H

#include "vw/core/types.h"

#include <array>

namespace vw {
template <typename T>
struct mat4 {
private:
    constexpr static size_t size_ = 16;
    std::array<T, size_> data_{};

public:
    mat4() = default;

    explicit mat4(const T* values) {
        memcpy(data_, values, size_ * sizeof(T));
    }

    mat4(const mat4& other)                    = default;
    auto operator=(const mat4& other) -> mat4& = default;
    mat4(mat4&& other)                         = default;
    auto operator=(mat4&& other) -> mat4&      = default;

    auto operator[](int row, int col) -> T& {
        return data_[(row * 4) + col];
    }

    auto operator[](int row, int col) const -> const T& {
        return data_[(row * 4) + col];
    }

    auto operator[](int index) -> T& {
        return data_[index];
    }

    auto operator[](int index) const -> const T& {
        return data_[index];
    }

    auto cptr() -> T* {
        return data_.data();
    }

    auto cptr() const -> const T* {
        return data_.data();
    }

    auto operator*(const mat4& other) const -> mat4 {
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

    auto operator==(const mat4& other) const -> bool {
        for (int i = 0; i < size_; ++i) {
            if (data_[i] != other.data_[i]) {
                return false;
            }
        }
        return true;
    }

    auto operator!=(const mat4& other) const -> bool {
        return !(*this == other);
    }
};

using mat4f = mat4<float32>;
using mat4d = mat4<float64>;
}  // namespace vw

#endif  // VW_MAT4_H
