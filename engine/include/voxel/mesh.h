#pragma once
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>

#include <voxel/types.h>
#include <voxel/model.h>
#include <voxel/buffer.h>

namespace voxel {
    class vulkan_context;

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

    // Структура для хранения данных меша без Vulkan буферов
    struct mesh_data {
        std::vector<vertex> vertices;
        std::vector<uint32> indices;
        
        mesh_data() = default;
        mesh_data(std::vector<vertex> v, std::vector<uint32> i) 
            : vertices(std::move(v)), indices(std::move(i)) {}
    };

    class mesh {
    public:
        mesh(std::shared_ptr<vulkan_context> context);
        ~mesh() = default;

        // Запретить копирование
        mesh(const mesh&) = delete;
        mesh& operator=(const mesh&) = delete;

        // Разрешить перемещение
        mesh(mesh&&) = default;
        mesh& operator=(mesh&&) = delete;

        void set_vertices(const std::vector<vertex>& vertices);
        void set_indices(const std::vector<uint32>& indices);
        void set_mesh_data(const mesh_data& data);

        void bind(VkCommandBuffer command_buffer);
        void draw(VkCommandBuffer command_buffer);
        void draw_indexed(VkCommandBuffer command_buffer);

        size_t get_vertex_count() const { return vertex_count_; }
        size_t get_index_count() const { return index_count_; }

    private:
        std::shared_ptr<vulkan_context> context_;
        std::unique_ptr<vertex_buffer> vertex_buffer_;
        std::unique_ptr<index_buffer> index_buffer_;
        size_t vertex_count_;
        size_t index_count_;
    };

    // Простой генератор мешей из воксельных моделей
    class simple_mesh_generator {
    public:
        static mesh generate_from_model(std::shared_ptr<vulkan_context> context, const std::shared_ptr<model>& model);
        static mesh_data generate_mesh_data(const std::shared_ptr<model>& model);
        
    private:
        static void add_cube_face(
            std::vector<vertex>& vertices,
            std::vector<uint32>& indices,
            const vec3f& position,
            int face_direction,
            uint32 color
        );
        static bool is_face_visible(const std::shared_ptr<model>& model, int x, int y, int z, int face_direction);
    };

    // Жадный генератор мешей из воксельных моделей
    class greedy_mesh_generator {
    public:
        static mesh generate_from_model(std::shared_ptr<vulkan_context> context, const std::shared_ptr<model>& model);
        static mesh_data generate_mesh_data(const std::shared_ptr<model>& model);
        
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
            uint32 color
        );
        static bool is_face_visible(const std::shared_ptr<model>& model, int x, int y, int z, int face_direction);
    };
}