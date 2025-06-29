#include <fstream>
#include <vector>
#include <stdexcept>

#include <voxel/shader.h>
#include <voxel/vulkan_context.h>

namespace {
    VkShaderStageFlagBits to_vulkan_shader_stage(voxel::shader_type type) {
        switch (type) {
            case voxel::shader_type::VERTEX:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case voxel::shader_type::FRAGMENT:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        throw std::runtime_error("Unknown shader type");
    }
}

namespace voxel {

shader::shader(std::shared_ptr<vulkan_context> context, const std::string& path, shader_type type) 
    : context_(context) {
    stage_ = to_vulkan_shader_stage(type);
    auto code = read_file(path);
    shader_module_ = create_shader_module(code);
}

shader::~shader() {
    vkDestroyShaderModule(context_->get_device(), shader_module_, nullptr);
}

VkPipelineShaderStageCreateInfo shader::get_stage_info() const {
    VkPipelineShaderStageCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    create_info.stage = stage_;
    create_info.module = shader_module_;
    create_info.pName = "main";
    return create_info;
}

VkShaderModule shader::create_shader_module(const std::vector<char>& code) {
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
    
    VkShaderModule shader_module;
    if (vkCreateShaderModule(context_->get_device(), &create_info, nullptr, &shader_module) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module");
    }
    
    return shader_module;
}

std::vector<char> shader::read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    
    size_t file_size = (size_t)file.tellg();
    std::vector<char> buffer(file_size);
    
    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();
    
    return buffer;
}

} // namespace voxel 