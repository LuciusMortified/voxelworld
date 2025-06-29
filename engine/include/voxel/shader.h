#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace voxel {
    class vulkan_context;

    enum class shader_type {
        VERTEX,
        FRAGMENT
    };

    class shader {
    public:
        shader(std::shared_ptr<vulkan_context> context, const std::string& path, shader_type type);
        ~shader();

        // Запретить копирование
        shader(const shader&) = delete;
        shader& operator=(const shader&) = delete;

        VkPipelineShaderStageCreateInfo get_stage_info() const;
        VkShaderModule get_module() const { return shader_module_; }

    private:
        VkShaderModule create_shader_module(const std::vector<char>& code);
        static std::vector<char> read_file(const std::string& filename);

        std::shared_ptr<vulkan_context> context_;
        VkShaderModule shader_module_;
        VkShaderStageFlagBits stage_;
    };
}