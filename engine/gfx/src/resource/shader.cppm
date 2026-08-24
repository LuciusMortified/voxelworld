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
    auto get_stage_info() const -> vk::PipelineShaderStageCreateInfo;

    [[nodiscard]]
    auto get_module() const -> vk::ShaderModule {
        return shader_module_;
    }

private:
    [[nodiscard]]
    auto create_shader_module(const std::vector<char>& code) const -> vk::ShaderModule;

    static std::vector<char> read_file(const std::string& filename);

    vulkan_context* context_;

    vk::ShaderModule shader_module_;
    vk::ShaderStageFlagBits stage_;
};
}  // namespace vw::gfx
