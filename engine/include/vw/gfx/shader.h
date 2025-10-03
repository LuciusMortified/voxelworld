#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace vw::gfx {
    class vulkan_context;

    enum class shader_type {
        VERTEX,
        FRAGMENT
    };

    class shader {
    public:
        shader(vulkan_context& context, const std::string& path, shader_type type);
        ~shader();

        shader(const shader&) = delete;
        shader& operator=(const shader&) = delete;

        [[nodiscard]]
        VkPipelineShaderStageCreateInfo get_stage_info() const;

        [[nodiscard]]
        VkShaderModule get_module() const { return shader_module_; }

    private:
        [[nodiscard]]
        VkShaderModule create_shader_module(const std::vector<char>& code) const;

        static std::vector<char> read_file(const std::string& filename);

        vulkan_context* context_;

        VkShaderModule shader_module_;
        VkShaderStageFlagBits stage_;
    };
} // namespace vw::gfx