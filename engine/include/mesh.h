#pragma once
#include <vector>
#include <memory>
#include "types.h"

namespace voxel {
    class vulkan_context;
    class vertex_buffer;
    class index_buffer;

    struct vertex {
        vec3f position;
        vec3f normal;
        uint32 color;

        vertex() : position(), normal(), color(0) {}
        vertex(const vec3f& pos, const vec3f& norm, uint32 col) 
            : position(pos), normal(norm), color(col) {}

        static std::vector<VkVertexInputBindingDescription> get_binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> get_attribute_descriptions();
    };

    class mesh {
    public:
        mesh(vulkan_context& context);
        ~mesh() = default;

        // Запретить копирование
        mesh(const mesh&) = delete;
        mesh& operator=(const mesh&) = delete;

        // Перемещение разрешено
        mesh(mesh&&) = default;
        mesh& operator=(mesh&&) = default;

        void set_vertices(const std::vector<vertex>& vertices);
        void set_indices(const std::vector<uint32>& indices);

        void bind(VkCommandBuffer command_buffer);
        void draw(VkCommandBuffer command_buffer);
        void draw_indexed(VkCommandBuffer command_buffer);

        size_t get_vertex_count() const { return vertex_count_; }
        size_t get_index_count() const { return index_count_; }

    private:
        vulkan_context& context_;
        std::unique_ptr<vertex_buffer> vertex_buffer_;
        std::unique_ptr<index_buffer> index_buffer_;
        size_t vertex_count_;
        size_t index_count_;
    };

    // Генератор мешей из воксельных моделей
    class mesh_generator {
    public:
        static mesh generate_from_model(vulkan_context& context, const class model& model);
        
    private:
        static void add_cube_face(std::vector<vertex>& vertices, std::vector<uint32>& indices,
                                const vec3f& position, int face_direction, uint32 color);
        static bool is_face_visible(const class model& model, int x, int y, int z, int face_direction);
    };
}