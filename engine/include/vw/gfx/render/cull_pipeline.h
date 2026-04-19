#pragma once

#ifndef VW_GFX_RENDER_CULL_PIPELINE_H
#define VW_GFX_RENDER_CULL_PIPELINE_H

#include <vulkan/vulkan.h>

#include <array>
#include <memory>
#include <span>

#include "vw/core/types.h"
#include "vw/core/vec4.h"
#include "vw/gfx/resource/buffer.h"
#include "vw/gfx/resource/combined_buffer.h"
#include "vw/gfx/resource/shader.h"
#include "vw/spatial/frustum.h"

namespace vw::gfx {

using vw::spatial::frustum;

class vulkan_context;

struct cull_frustum_ubo {
    vec4f planes[30];
    uint32 pass_count;
    uint32 pad[3];
};

class cull_pipeline {
public:
    static constexpr uint32 max_frames_in_flight = 2;

    explicit cull_pipeline(
        vulkan_context& context,
        VkDescriptorPool descriptor_pool
    );
    ~cull_pipeline();

    cull_pipeline(const cull_pipeline&)            = delete;
    auto operator=(const cull_pipeline&) -> cull_pipeline& = delete;
    cull_pipeline(cull_pipeline&&)                 = delete;
    auto operator=(cull_pipeline&&) -> cull_pipeline&      = delete;

    void update_frustums(
        uint32 frame_index,
        const frustum& view_frustum,
        std::span<const frustum> shadow_frustums
    );

    void dispatch(
        VkCommandBuffer cmd,
        combined_buffer& buffer,
        uint32 frame_index
    );

    [[nodiscard]] auto get_buffer_descriptor_set_layout() const -> VkDescriptorSetLayout {
        return buffer_descriptor_set_layout_;
    }

private:
    void create_descriptor_set_layouts_();
    void create_pipeline_();
    void create_frustum_ubos_();

    vulkan_context* context_;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;

    std::unique_ptr<shader> compute_shader_;
    VkPipeline compute_pipeline_                      = VK_NULL_HANDLE;
    VkPipelineLayout compute_pipeline_layout_         = VK_NULL_HANDLE;
    VkDescriptorSetLayout frustum_descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout buffer_descriptor_set_layout_  = VK_NULL_HANDLE;

    std::array<std::unique_ptr<uniform_buffer>, max_frames_in_flight> frustum_ubos_;
    std::array<VkDescriptorSet, max_frames_in_flight> frustum_descriptor_sets_{};
};

}  // namespace vw::gfx

#include "vw/gfx/render/cull_pipeline.inl.h"

#endif  // VW_GFX_RENDER_CULL_PIPELINE_H
