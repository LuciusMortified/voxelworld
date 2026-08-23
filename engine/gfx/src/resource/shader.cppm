export module vw.gfx:resource.shader;

import std;

import vw.core;
import vulkan;

export namespace vw::gfx {
class vulkan_context;

enum class shader_type { VERTEX, FRAGMENT, COMPUTE };

class shader {
public:
    shader(vulkan_context& context, const std::string& path, shader_type type);
    ~shader();

    shader(const shader&)            = delete;
    shader& operator=(const shader&) = delete;

    shader(shader&&)            = delete;
    shader& operator=(shader&&) = delete;

    [[nodiscard]]
    vk::PipelineShaderStageCreateInfo get_stage_info() const;

    [[nodiscard]]
    vk::ShaderModule get_module() const {
        return shader_module_;
    }

private:
    [[nodiscard]]
    vk::ShaderModule create_shader_module(const std::vector<char>& code) const;

    static std::vector<char> read_file(const std::string& filename);

    vulkan_context* context_;

    vk::ShaderModule shader_module_;
    vk::ShaderStageFlagBits stage_;
};
}  // namespace vw::gfx
