#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace voxel {
    class vulkan_context;

    class shader {
    public:
        shader(vulkan_context& context, const std::string& vertex_path, const std::string& fragment_path);
        ~shader();

        // Запретить копирование
        shader(const shader&) = delete;
        shader& operator=(const shader&) = delete;

        VkShaderModule get_vertex_module() const { return vertex_module_; }
        VkShaderModule get_fragment_module() const { return fragment_module_; }

        std::vector<VkPipelineShaderStageCreateInfo> get_stage_infos() const;

    private:
        VkShaderModule create_shader_module(const std::vector<char>& code);
        std::vector<char> read_file(const std::string& filename);

        vulkan_context& context_;
        VkShaderModule vertex_module_;
        VkShaderModule fragment_module_;
    };

    // Управление шейдерными программами
    class shader_program {
    public:
        shader_program(vulkan_context& context);
        ~shader_program();

        void load_vertex_shader(const std::string& path);
        void load_fragment_shader(const std::string& path);
        void load_geometry_shader(const std::string& path);

        std::vector<VkPipelineShaderStageCreateInfo> get_stage_infos() const;

    private:
        vulkan_context& context_;
        VkShaderModule vertex_module_;
        VkShaderModule fragment_module_;
        VkShaderModule geometry_module_;
    };
}