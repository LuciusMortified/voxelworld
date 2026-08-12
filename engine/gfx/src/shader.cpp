module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {
namespace {

auto to_vulkan_shader_stage(shader_type type) -> vk::ShaderStageFlagBits {
    switch (type) {
        case shader_type::VERTEX:
            return vk::ShaderStageFlagBits::eVertex;
        case shader_type::FRAGMENT:
            return vk::ShaderStageFlagBits::eFragment;
        case shader_type::COMPUTE:
            return vk::ShaderStageFlagBits::eCompute;
    }
    throw std::runtime_error("unknown shader type");
}

}  // namespace

shader::shader(vulkan_context& context, const std::string& path, shader_type type) : context_(&context) {
    stage_ = to_vulkan_shader_stage(type);
    const auto code = read_file(path);
    shader_module_ = create_shader_module(code);
}

shader::~shader() {
    context_->get_device().destroyShaderModule(shader_module_);
}

auto shader::get_stage_info() const -> vk::PipelineShaderStageCreateInfo {
    return {
        .stage  = stage_,
        .module = shader_module_,
        .pName  = "main",
    };
}

auto shader::create_shader_module(const std::vector<char>& code) const -> vk::ShaderModule {
    return vk_must(
        context_->get_device().createShaderModule({
            .codeSize = code.size(),
            .pCode    = reinterpret_cast<const uint32*>(code.data()),
        }),
        "create shader module"
    );
}

auto shader::read_file(const std::string& filename) -> std::vector<char> {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    const std::streamsize file_size = file.tellg();
    std::vector<char> buffer(static_cast<size_t>(file_size));

    file.seekg(0);
    file.read(buffer.data(), file_size);

    return buffer;
}

}// namespace vw::gfx
