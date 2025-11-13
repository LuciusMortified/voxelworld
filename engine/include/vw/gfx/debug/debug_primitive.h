#pragma once

#ifndef VW_GFX_DEBUG_PRIMITIVE_H
#define VW_GFX_DEBUG_PRIMITIVE_H

#include <vector>

#include <vulkan/vulkan_core.h>

#include "vw/core.h"
#include "vw/core/transform.h"

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
    void add_line(const vec3f& begin, const vec3f& end, color clr = colors::red);

    void add_box(const vw::transform& transform, const vec3f& size, color clr = colors::green);
    void add_box(const vec3f& pos, const vec3f& size, color clr = colors::green);

    void add_grid(const vw::transform& transform, float cell_size, int cols, int rows, color clr = colors::gray);
    void add_grid(const vec3f& pos, float cel_size, int cols, int rows, color clr = colors::gray);

    [[nodiscard]]
    auto get_vertices() const -> const std::vector<debug_vertex>&;

    [[nodiscard]]
    auto is_empty() const -> bool;

    void clear();

private:
    std::vector<debug_vertex> vertices_;
};

}  // namespace vw::gfx

#include "vw/gfx/debug/debug_primitive.inl.h"

#endif  // VW_GFX_DEBUG_PRIMITIVE_H
