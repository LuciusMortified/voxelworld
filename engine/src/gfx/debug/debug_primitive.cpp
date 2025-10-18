#include "vw/gfx/debug/debug_primitive.h"

#include "vw/core/vec3.h"
#include "vw/core/color.h"

#include <cstddef>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace vw::gfx {

auto debug_vertex::get_binding_descriptions() -> std::vector<VkVertexInputBindingDescription> {
    std::vector<VkVertexInputBindingDescription> bindings(1);

    bindings[0].binding   = 0;
    bindings[0].stride    = sizeof(debug_vertex);
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindings;
}

auto debug_vertex::get_attribute_descriptions() -> std::vector<VkVertexInputAttributeDescription> {
    std::vector<VkVertexInputAttributeDescription> attributes(2);

    // Position
    attributes[0].binding  = 0;
    attributes[0].location = 0;
    attributes[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset   = offsetof(debug_vertex, pos);

    // Color
    attributes[1].binding  = 0;
    attributes[1].location = 1;
    attributes[1].format   = VK_FORMAT_R32_UINT;
    attributes[1].offset   = offsetof(debug_vertex, col);

    return attributes;
}

void debug_primitives::clear() {
    vertices_.clear();
}

void debug_primitives::add_line(const vec3f& begin, const vec3f& end, color col) {
    vertices_.emplace_back(begin, col);
    vertices_.emplace_back(end, col);
}

auto debug_primitives::get_vertices() const -> const std::vector<debug_vertex>& {
    return vertices_;
}

}  // namespace vw::gfx