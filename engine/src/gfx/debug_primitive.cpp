#include "vw/gfx/debug_primitive.h"

namespace vw::gfx {

std::vector<VkVertexInputBindingDescription>
debug_vertex::get_binding_descriptions() {
    std::vector<VkVertexInputBindingDescription> bindings(1);

    bindings[0].binding   = 0;
    bindings[0].stride    = sizeof(debug_vertex);
    bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindings;
}

std::vector<VkVertexInputAttributeDescription>
debug_vertex::get_attribute_descriptions() {
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

void debug_primitives::add_line(const vec3f& a, const vec3f& b, color col) {
    vertices_.push_back(debug_vertex{a, col});
    vertices_.push_back(debug_vertex{b, col});
}

const std::vector<debug_vertex>& debug_primitives::get_vertices() const {
    return vertices_;
}

}  // namespace vw::gfx