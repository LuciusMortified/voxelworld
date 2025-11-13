#pragma once

#ifndef VW_GFX_DEBUG_PRIMITIVE_INL_H
#define VW_GFX_DEBUG_PRIMITIVE_INL_H

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <vector>

#include "vw/core/color.h"
#include "vw/core/mat4.h"
#include "vw/core/vec3.h"
#include "vw/gfx/debug/debug_primitive.h"

namespace vw::gfx {

inline auto debug_vertex::get_binding_descriptions()
    -> std::vector<VkVertexInputBindingDescription> {
    std::vector<VkVertexInputBindingDescription> bindings(1);

    bindings[0].binding   = 0;
    bindings[0].stride    = sizeof(debug_vertex);
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindings;
}

inline auto debug_vertex::get_attribute_descriptions()
    -> std::vector<VkVertexInputAttributeDescription> {
    std::vector<VkVertexInputAttributeDescription> attributes(2);

    attributes[0].binding  = 0;
    attributes[0].location = 0;
    attributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset   = offsetof(debug_vertex, pos);

    attributes[1].binding  = 0;
    attributes[1].location = 1;
    attributes[1].format   = VK_FORMAT_R32_UINT;
    attributes[1].offset   = offsetof(debug_vertex, col);

    return attributes;
}

inline void debug_primitives::clear() {
    vertices_.clear();
}

inline void debug_primitives::add_line(
    const vec3f& begin, const vec3f& end, color clr
) {
    vertices_.emplace_back(begin, clr);
    vertices_.emplace_back(end, clr);
}

inline auto debug_primitives::get_vertices() const -> const std::vector<debug_vertex>& {
    return vertices_;
}

inline void debug_primitives::add_box(
    const transform& transform, const vec3f& size, color clr
) {
    const mat4f matrix = transform.calc_matrix();

    const vec3f p0 = matrix * vec3f{0.0f, 0.0f, 0.0f};
    const vec3f p1 = matrix * vec3f{size.x, 0.0f, 0.0f};
    const vec3f p2 = matrix * vec3f{size.x, 0.0f, size.z};
    const vec3f p3 = matrix * vec3f{0.0f, 0.0f, size.z};
    const vec3f p4 = matrix * vec3f{0.0f, size.y, 0.0f};
    const vec3f p5 = matrix * vec3f{size.x, size.y, 0.0f};
    const vec3f p6 = matrix * vec3f{size.x, size.y, size.z};
    const vec3f p7 = matrix * vec3f{0.0f, size.y, size.z};

    add_line(p0, p1, clr);
    add_line(p1, p2, clr);
    add_line(p2, p3, clr);
    add_line(p3, p0, clr);

    add_line(p4, p5, clr);
    add_line(p5, p6, clr);
    add_line(p6, p7, clr);
    add_line(p7, p4, clr);

    add_line(p0, p4, clr);
    add_line(p1, p5, clr);
    add_line(p2, p6, clr);
    add_line(p3, p7, clr);
}

inline void debug_primitives::add_grid(
    const transform& transform, float cell_size, int cols, int rows, color clr
) {
    const mat4f matrix = transform.calc_matrix();

    const float cols_size = cell_size * static_cast<float>(cols);
    for (int i = 0; i <= cols; ++i) {
        const vec3f start = matrix * vec3f{static_cast<float>(i) * cell_size, 0.0f, 0.0f};
        const vec3f end   = matrix * vec3f{static_cast<float>(i) * cell_size, 0.0f, cols_size};
        add_line(start, end, clr);
    }

    const float rows_size = cell_size * static_cast<float>(rows);
    for (int j = 0; j <= rows; ++j) {
        const vec3f start = matrix * vec3f{0.0f, 0.0f, static_cast<float>(j) * cell_size};
        const vec3f end   = matrix * vec3f{rows_size, 0.0f, static_cast<float>(j) * cell_size};
        add_line(start, end, clr);
    }
}

inline void debug_primitives::add_box(
    const vec3f& pos, const vec3f& size, color clr
) {
    vw::transform t;
    t.set_position(pos);
    add_box(t, size, clr);
}

inline void debug_primitives::add_grid(
    const vec3f& pos, float cel_size, int cols, int rows, color clr
) {
    vw::transform t;
    t.set_position(pos);
    add_grid(t, cel_size, cols, rows, clr);
}

inline auto debug_primitives::is_empty() const -> bool {
    return vertices_.empty();
}

}  // namespace vw::gfx

#endif  // VW_GFX_DEBUG_PRIMITIVE_INL_H
