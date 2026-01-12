#pragma once

#ifndef VW_GFX_MESH_H
#define VW_GFX_MESH_H

#include <vulkan/vulkan.h>

#include <memory>
#include <vector>

#include "vw/core/types.h"
#include "vw/core/vec3.h"
#include "vw/gfx/model/model.h"
#include "vw/gfx/resource/buffer.h"

namespace vw::gfx {
struct vertex {
    vec3f position;
    vec3f normal;
    uint32 color;

    vertex() : color(0) {}
    vertex(
        const vec3f& pos, const vec3f& norm, uint32 col
    )
        : position(pos), normal(norm), color(col) {}

    [[nodiscard]] static auto get_binding_descriptions()
        -> std::vector<VkVertexInputBindingDescription>;

    [[nodiscard]] static auto get_attribute_descriptions()
        -> std::vector<VkVertexInputAttributeDescription>;
};

struct mesh {
    std::vector<vertex> vertices;
    std::vector<uint32> indices;
};

class simple_mesh_generator {
public:
    [[nodiscard]]
    static mesh generate_mesh_data(std::shared_ptr<model> model);

private:
    static void add_cube_face(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const vec3f& position,
        int face_direction,
        color color
    );

    [[nodiscard]]
    static bool is_face_visible(
        const std::shared_ptr<model>& model, int x, int y, int z, int face_direction
    );
};

class greedy_mesh_generator {
public:
    [[nodiscard]]
    static mesh generate_mesh_data(std::shared_ptr<model> model);

private:
    static void generate_face_quads(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const std::shared_ptr<model>& model,
        int face_direction
    );
    static void add_quad(
        std::vector<vertex>& vertices,
        std::vector<uint32>& indices,
        const vec3f& min_pos,
        const vec3f& max_pos,
        int face_direction,
        color color
    );

    [[nodiscard]]
    static bool is_face_visible(
        const std::shared_ptr<model>& model, int x, int y, int z, int face_direction
    );
};
}  // namespace vw::gfx

#include "vw/gfx/resource/mesh.inl.h"

#endif  // VW_GFX_MESH_H
