#pragma once

#ifndef VW_GFX_DEBUG_PRIMITIVE_H
#define VW_GFX_DEBUG_PRIMITIVE_H

#include <vector>

#include <vulkan/vulkan_core.h>

#include "vw/core.h"

namespace vw::gfx {

class vulkan_context;

struct debug_vertex {
    vec3f pos;
    color col;

    explicit debug_vertex(vec3f pos, color col = colors::red) : pos(pos), col(col) {}

    [[nodiscard]]
    static auto get_binding_descriptions() -> std::vector<VkVertexInputBindingDescription>;

    [[nodiscard]]
    static auto get_attribute_descriptions() -> std::vector<VkVertexInputAttributeDescription>;
};

class debug_primitives {
public:
    void add_line(const vec3f& begin, const vec3f& end, color col = colors::red);

    void add_box(const vec3f& origin, const vec3f& size, color col = colors::green) {
        const vec3f p0 = origin;
        const vec3f p1 = origin + vec3f{size.x, 0.0f, 0.0f};
        const vec3f p2 = origin + vec3f{size.x, 0.0f, size.z};
        const vec3f p3 = origin + vec3f{0.0f, 0.0f, size.z};
        const vec3f p4 = origin + vec3f{0.0f, size.y, 0.0f};
        const vec3f p5 = origin + vec3f{size.x, size.y, 0.0f};
        const vec3f p6 = origin + vec3f{size.x, size.y, size.z};
        const vec3f p7 = origin + vec3f{0.0f, size.y, size.z};

        // Bottom face
        add_line(p0, p1, col);
        add_line(p1, p2, col);
        add_line(p2, p3, col);
        add_line(p3, p0, col);

        // Top face
        add_line(p4, p5, col);
        add_line(p5, p6, col);
        add_line(p6, p7, col);
        add_line(p7, p4, col);

        // Vertical edges
        add_line(p0, p4, col);
        add_line(p1, p5, col);
        add_line(p2, p6, col);
        add_line(p3, p7, col);
    }

    void add_grid(const vec3f& origin, float cel_size, int lines, color col = colors::gray) {
        const float size = cel_size * static_cast<float>(lines);
        for (int i = -lines; i <= lines; ++i) {
            add_line(
                origin + vec3f{static_cast<float>(i) * cel_size, 0.0f, -size},
                origin + vec3f{static_cast<float>(i) * cel_size, 0.0f, size},
                col
            );
            add_line(
                origin + vec3f{-size, 0.0f, static_cast<float>(i) * cel_size},
                origin + vec3f{size, 0.0f, static_cast<float>(i) * cel_size},
                col
            );
        }
    }

    [[nodiscard]]
    auto get_vertices() const -> const std::vector<debug_vertex>&;

    [[nodiscard]]
    auto is_empty() const -> bool {
        return vertices_.empty();
    }

    void clear();

private:
    std::vector<debug_vertex> vertices_;
};

}  // namespace vw::gfx

#endif  // VW_GFX_DEBUG_PRIMITIVE_H
