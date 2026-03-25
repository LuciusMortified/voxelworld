#pragma once

#ifndef VW_GFX_RENDER_RENDERER_INL_H
#define VW_GFX_RENDER_RENDERER_INL_H

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "vw/core/timing.h"

namespace vw::gfx {

template <typename C>
renderer<C>::renderer(
    vulkan_context& context, window& window, const block_registry& registry
)
    : context_(&context), window_(&window), block_registry_(&registry) {
    vertex_shader_ =
        std::make_unique<shader>(*context_, "shaders/voxel.vert.spv", shader_type::VERTEX);
    fragment_shader_ =
        std::make_unique<shader>(*context_, "shaders/voxel.frag.spv", shader_type::FRAGMENT);

    debug_vertex_shader_ =
        std::make_unique<shader>(*context_, "shaders/debug.vert.spv", shader_type::VERTEX);
    debug_fragment_shader_ =
        std::make_unique<shader>(*context_, "shaders/debug.frag.spv", shader_type::FRAGMENT);

    shadow_vertex_shader_ =
        std::make_unique<shader>(*context_, "shaders/shadow.vert.spv", shader_type::VERTEX);
    shadow_fragment_shader_ =
        std::make_unique<shader>(*context_, "shaders/shadow.frag.spv", shader_type::FRAGMENT);

    constexpr VkDeviceSize initial_size = 512 * 2 * sizeof(debug_vertex);
    debug_vertex_buffer_                = std::make_unique<vertex_buffer>(*context_, initial_size);

    shadow_map_ = std::make_unique<shadow_map>(*context_);

    msaa_samples_ = get_max_usable_sample_count();

    create_swapchain();
    create_image_views();
    create_color_resources();
    create_depth_resources();
    create_render_pass();
    create_descriptor_set_layouts();
    create_point_lights_descriptor_set_layout();
    create_palette_descriptor_set_layout();
    create_graphics_pipeline();
    create_wireframe_pipeline();
    create_shadow_pipeline();
    create_debug_pipeline();
    create_framebuffers();
    create_command_buffers();
    create_sync_objects();
    create_uniform_buffers();
    create_shadow_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
    create_shadow_descriptor_sets();
    create_shadow_map_descriptor_sets();

    create_imgui_descriptor_pool();
    init_imgui();

    cull_pipeline_ = std::make_unique<cull_pipeline>(*context_, descriptor_pool_);

    combined_buffer_pool_ = std::make_unique<combined_buffer_pool_type>(
        *context_, descriptor_pool_, storage_descriptor_set_layout_,
        cull_pipeline_->get_buffer_descriptor_set_layout()
    );

    light_buffer_ = std::make_unique<light_buffer_type>(
        *context_, descriptor_pool_, point_lights_descriptor_set_layout_
    );

    palette_buffer_ = std::make_unique<palette_buffer>(
        *context_, descriptor_pool_, palette_descriptor_set_layout_, *block_registry_
    );
}

template <typename C>
renderer<C>::~renderer() {
    wait_idle();

    combined_buffer_pool_.reset();
    cull_pipeline_.reset();
    palette_buffer_.reset();
    light_buffer_.reset();
    shadow_map_.reset();

    cleanup_imgui();

    cleanup_swapchain();
    cleanup_color_resources();
    cleanup_depth_resources();
    cleanup_pipelines();
    cleanup_shadow_pipeline();
    cleanup_debug_pipeline();
    cleanup_point_lights_resources();
    cleanup_palette_resources();
    cleanup_descriptor_set_layouts();
    cleanup_render_pass();
    cleanup_descriptor_pool();
}

template <typename C>
void renderer<C>::begin_frame() {
    // Ждем завершения предыдущего кадра
    vkWaitForFences(
        context_->get_device(), 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX
    );

    // Сбрасываем fence для рендеринга перед использованием
    vkResetFences(context_->get_device(), 1, &in_flight_fences_[current_frame_]);

    // Получаем следующий image из swapchain (используем семафор)
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(
        context_->get_device(),
        swapchain_,
        UINT64_MAX,
        image_available_semaphores_[current_frame_],
        VK_NULL_HANDLE,
        &image_index
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    current_image_index_ = image_index;

    // Ждем завершения предыдущего использования этого изображения
    if (images_in_flight_[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(
            context_->get_device(), 1, &images_in_flight_[image_index], VK_TRUE, UINT64_MAX
        );
    }

    // Связываем fence с изображением
    images_in_flight_[image_index] = in_flight_fences_[current_frame_];

    // Подготовка нового кадра ImGui
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

template <typename C>
void renderer<C>::end_frame() {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[]      = {image_available_semaphores_[current_frame_]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount     = 1;
    submit_info.pWaitSemaphores        = wait_semaphores;
    submit_info.pWaitDstStageMask      = wait_stages;
    submit_info.commandBufferCount     = 1;
    submit_info.pCommandBuffers        = &command_buffers_[current_image_index_];

    VkSemaphore signal_semaphores[]  = {render_finished_semaphores_[current_image_index_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    if (vkQueueSubmit(
            context_->get_graphics_queue(), 1, &submit_info, in_flight_fences_[current_frame_]
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffers!");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;

    VkSwapchainKHR swapchains[] = {swapchain_};
    present_info.swapchainCount = 1;
    present_info.pSwapchains    = swapchains;
    present_info.pImageIndices  = &current_image_index_;

    VkResult result = vkQueuePresentKHR(context_->get_present_queue(), &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_) {
        framebuffer_resized_ = false;
        recreate_swapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    debug_primitives_.clear();

    stats_.draw_call_count = draw_call_count_;
    draw_call_count_       = 0;

    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

template <typename C>
const renderer_stats& renderer<C>::get_stats() const {
    stats_.combined_buffers = combined_buffer_pool_->get_stats();
    return stats_;
}

template <typename C>
auto renderer<C>::get_directional_light_settings() -> directional_light_settings& {
    return directional_light_settings_;
}

template <typename C>
auto renderer<C>::get_fog_settings() -> fog_settings& {
    return fog_settings_;
}

template <typename C>
void renderer<C>::set_clear_color(
    float r, float g, float b, float a
) {
    clear_color_ = {r, g, b, a};
}

template <typename C>
void renderer<C>::set_clear_color(
    vec4f color
) {
    clear_color_ = color;
}

template <typename C>
void renderer<C>::wait_idle() const {
    vkDeviceWaitIdle(context_->get_device());
}

template <typename C>
void renderer<C>::handle_resize() {
    framebuffer_resized_ = true;
}

template <typename C>
void renderer<C>::set_render_mode(
    render_mode mode
) {
    current_render_mode_ = mode;
}

template <typename WC>
render_mode renderer<WC>::get_render_mode() const {
    return current_render_mode_;
}

template <typename C>
void renderer<C>::render(
    world_type& world, camera& camera
) {
    // Начинаем запись в command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(command_buffers_[current_image_index_], &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    stats_.timing.shadow_map_update_ms = measure_ms([&] {
        shadow_map_->update(camera, directional_light_settings_.direction);
    });

    const auto& cascade_frustums = shadow_map_->get_cascade_frustums();

    stats_.timing.buffer_pool_update_ms = measure_ms([&] {
        combined_buffer_pool_->update(
            world, camera, command_buffers_[current_image_index_]
        );
    });

    stats_.timing.compute_cull_ms = measure_ms([&] {
        {
            VkMemoryBarrier barrier{};
            barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask =                    //
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |  //
                VK_ACCESS_INDEX_READ_BIT |             //
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT |  //
                VK_ACCESS_SHADER_READ_BIT;
            constexpr auto stage_mask =                //
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |   //
                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |  //
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |  //
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            vkCmdPipelineBarrier(
                command_buffers_[current_image_index_],
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                stage_mask,
                0, 1, &barrier, 0, nullptr, 0, nullptr
            );
        }

        const frustum& view_frustum = camera.get_frustum();
        cull_pipeline_->update_frustums(current_frame_, view_frustum, cascade_frustums);

        for (const auto& buffer : combined_buffer_pool_->get_buffers()) {
            if (!buffer->is_empty()) {
                cull_pipeline_->dispatch(
                    command_buffers_[current_image_index_], *buffer, current_frame_
                );
            }
        }

        {
            VkMemoryBarrier compute_barrier{};
            compute_barrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            compute_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            compute_barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            vkCmdPipelineBarrier(
                command_buffers_[current_image_index_],
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                0, 1, &compute_barrier, 0, nullptr, 0, nullptr
            );
        }
    });

    stats_.timing.shadow_pass_ms = measure_ms([&] {
        render_shadow_pass(world, camera);
    });

    stats_.timing.world_pass_ms = measure_ms([&] {
        render_world_pass(world, camera);
    });

    if (vkEndCommandBuffer(command_buffers_[current_image_index_]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

template <typename C>
void renderer<C>::draw_line(
    const vec3f& a, const vec3f& b, color col
) {
    debug_primitives_.add_line(a, b, col);
}

template <typename C>
void renderer<C>::draw_box(
    const mat4f& matrix, const vec3f& size, const color col
) {
    debug_primitives_.add_box(matrix, size, col);
}

template <typename C>
void renderer<C>::draw_box(
    const transform& transform, const vec3f& size, color col
) {
    debug_primitives_.add_box(transform, size, col);
}

template <typename C>
void renderer<C>::draw_box(
    const vec3f& position, const vec3f& size, color col
) {
    debug_primitives_.add_box(position, size, col);
}

template <typename C>
void renderer<C>::draw_grid(
    const mat4f& matrix, float cell_size, int cols, int rows, color clr
) {
    debug_primitives_.add_grid(matrix, cell_size, cols, rows, clr);
}

template <typename C>
void renderer<C>::draw_grid(
    const transform& transform, float cell_size, int cols, int rows, color clr
) {
    debug_primitives_.add_grid(transform, cell_size, cols, rows, clr);
}

template <typename C>
void renderer<C>::draw_grid(
    const vec3f& position, float cell_size, int cols, int rows, color clr
) {
    debug_primitives_.add_grid(position, cell_size, cols, rows, clr);
}

template <typename C>
void renderer<C>::create_swapchain() {
    auto swapchain_support = context_->query_swapchain_support_();

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode     = choose_swap_present_mode(swapchain_support.present_modes);
    VkExtent2D extent                 = choose_swap_extent(swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 &&
        image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface          = context_->get_surface();
    create_info.minImageCount    = image_count;
    create_info.imageFormat      = surface_format.format;
    create_info.imageColorSpace  = surface_format.colorSpace;
    create_info.imageExtent      = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform     = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode      = present_mode;
    create_info.clipped          = VK_TRUE;
    create_info.oldSwapchain     = VK_NULL_HANDLE;

    auto queue_families = context_->get_queue_families();
    if (queue_families.graphics_family != queue_families.present_family) {
        uint32_t queue_family_indices[] = {
            queue_families.graphics_family.value(), queue_families.present_family.value()
        };
        create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices   = queue_family_indices;
    }

    if (vkCreateSwapchainKHR(context_->get_device(), &create_info, nullptr, &swapchain_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(context_->get_device(), swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);

    vkGetSwapchainImagesKHR(
        context_->get_device(), swapchain_, &image_count, swapchain_images_.data()
    );

    swapchain_image_format_ = surface_format.format;
    swapchain_extent_       = extent;
}

template <typename C>
void renderer<C>::create_image_views() {
    swapchain_image_views_.resize(swapchain_images_.size());

    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        VkImageViewCreateInfo view_info{};
        view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image                           = swapchain_images_[i];
        view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format                          = swapchain_image_format_;
        view_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(
                context_->get_device(), &view_info, nullptr, &swapchain_image_views_[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views");
        }
    }
}

template <typename C>
void renderer<C>::create_color_resources() {
    create_image(
        color_image_,
        color_image_memory_,
        swapchain_extent_,
        swapchain_image_format_,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        msaa_samples_
    );
    color_image_view_ =
        create_image_view(color_image_, swapchain_image_format_, VK_IMAGE_ASPECT_COLOR_BIT);
}

template <typename C>
void renderer<C>::create_depth_resources() {
    VkFormat depth_format = find_depth_format();

    create_image(
        depth_image_,
        depth_image_memory_,
        swapchain_extent_,
        depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        msaa_samples_
    );
    depth_image_view_ = create_image_view(depth_image_, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

template <typename C>
void renderer<C>::create_render_pass() {
    // Attachment 0: MSAA color (render target)
    VkAttachmentDescription color_attachment{};
    color_attachment.format         = swapchain_image_format_;
    color_attachment.samples        = msaa_samples_;
    color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Attachment 1: MSAA depth
    VkAttachmentDescription depth_attachment{};
    depth_attachment.format         = find_depth_format();
    depth_attachment.samples        = msaa_samples_;
    depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Attachment 2: resolve target (swapchain image, 1x)
    VkAttachmentDescription color_resolve_attachment{};
    color_resolve_attachment.format         = swapchain_image_format_;
    color_resolve_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    color_resolve_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_resolve_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    color_resolve_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_resolve_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    color_resolve_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_resolve_ref{};
    color_resolve_ref.attachment = 2;
    color_resolve_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_3d    = {};
    subpass_3d.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_3d.colorAttachmentCount    = 1;
    subpass_3d.pColorAttachments       = &color_attachment_ref;
    subpass_3d.pDepthStencilAttachment = &depth_attachment_ref;
    subpass_3d.pResolveAttachments     = nullptr;

    VkSubpassDescription subpass_debug    = {};
    subpass_debug.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_debug.colorAttachmentCount    = 1;
    subpass_debug.pColorAttachments       = &color_attachment_ref;
    subpass_debug.pDepthStencilAttachment = &depth_attachment_ref;
    subpass_debug.pResolveAttachments     = nullptr;

    VkSubpassDescription subpass_imgui    = {};
    subpass_imgui.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_imgui.colorAttachmentCount    = 1;
    subpass_imgui.pColorAttachments       = &color_attachment_ref;
    subpass_imgui.pDepthStencilAttachment = nullptr;
    subpass_imgui.pResolveAttachments     = &color_resolve_ref;

    VkSubpassDescription subpasses[] = {subpass_3d, subpass_debug, subpass_imgui};

    VkSubpassDependency dependency_3d = {};
    dependency_3d.srcSubpass          = VK_SUBPASS_EXTERNAL;
    dependency_3d.dstSubpass          = 0;
    dependency_3d.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_3d.srcAccessMask       = 0;
    dependency_3d.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_3d.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependency_debug = {};
    dependency_debug.srcSubpass          = 0;
    dependency_debug.dstSubpass          = 1;
    dependency_debug.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_debug.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency_debug.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_debug.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependency_imgui = {};
    dependency_imgui.srcSubpass          = 1;
    dependency_imgui.dstSubpass          = 2;
    dependency_imgui.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_imgui.srcAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency_imgui.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency_imgui.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[] = {dependency_3d, dependency_debug, dependency_imgui};

    VkAttachmentDescription attachments[] = {
        color_attachment, depth_attachment, color_resolve_attachment
    };

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 3;
    render_pass_info.pAttachments    = attachments;
    render_pass_info.subpassCount    = 3;
    render_pass_info.pSubpasses      = subpasses;
    render_pass_info.dependencyCount = 3;
    render_pass_info.pDependencies   = dependencies;

    if (vkCreateRenderPass(context_->get_device(), &render_pass_info, nullptr, &render_pass_) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass");
    }
}

template <typename C>
void renderer<C>::create_descriptor_set_layouts() {
    // Uniform buffer descriptor set layout (set 0, binding 0)
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding         = 0;
    ubo_layout_binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo ubo_layout_info{};
    ubo_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ubo_layout_info.bindingCount = 1;
    ubo_layout_info.pBindings    = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(
            context_->get_device(), &ubo_layout_info, nullptr, &uniform_descriptor_set_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create uniform descriptor set layout");
    }

    // Storage buffer descriptor set layout (set 1: binding 0 = model matrices, binding 1 = normal matrices)
    std::array<VkDescriptorSetLayoutBinding, 2> storage_layout_bindings{};
    storage_layout_bindings[0].binding            = 0;
    storage_layout_bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storage_layout_bindings[0].descriptorCount    = 1;
    storage_layout_bindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    storage_layout_bindings[0].pImmutableSamplers = nullptr;

    storage_layout_bindings[1].binding            = 1;
    storage_layout_bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    storage_layout_bindings[1].descriptorCount    = 1;
    storage_layout_bindings[1].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    storage_layout_bindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo storage_layout_info{};
    storage_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    storage_layout_info.bindingCount = storage_layout_bindings.size();
    storage_layout_info.pBindings    = storage_layout_bindings.data();

    if (vkCreateDescriptorSetLayout(
            context_->get_device(), &storage_layout_info, nullptr, &storage_descriptor_set_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create storage descriptor set layout");
    }

    // Shadow map descriptor set layout (set 2, binding 0)
    VkDescriptorSetLayoutBinding shadow_layout_binding{};
    shadow_layout_binding.binding            = 0;
    shadow_layout_binding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadow_layout_binding.descriptorCount    = 1;
    shadow_layout_binding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadow_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo shadow_layout_info{};
    shadow_layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    shadow_layout_info.bindingCount = 1;
    shadow_layout_info.pBindings    = &shadow_layout_binding;

    if (vkCreateDescriptorSetLayout(
            context_->get_device(), &shadow_layout_info, nullptr, &shadow_descriptor_set_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shadow descriptor set layout");
    }
}

template <typename C>
void renderer<C>::create_graphics_pipeline() {
    // Используем уже созданные шейдеры
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_->get_stage_info(), fragment_shader_->get_stage_info()
    };

    // Vertex input state
    auto binding_description    = vertex::get_binding_descriptions();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32_t>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    // Rasterizer state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = msaa_samples_;

    // Color blend state
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable   = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments    = &color_blend_attachment;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable  = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp   = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable     = VK_FALSE;

    std::array<VkDescriptorSetLayout, 5> descriptor_set_layouts = {
        uniform_descriptor_set_layout_,
        storage_descriptor_set_layout_,
        shadow_descriptor_set_layout_,
        point_lights_descriptor_set_layout_,
        palette_descriptor_set_layout_
    };

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size());
    pipeline_layout_info.pSetLayouts    = descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges    = nullptr;

    if (vkCreatePipelineLayout(
            context_->get_device(), &pipeline_layout_info, nullptr, &pipeline_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipeline_info.stageCount          = 2;
    pipeline_info.pStages             = shader_stages;
    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState   = &multisampling;
    pipeline_info.pColorBlendState    = &color_blending;
    pipeline_info.pDepthStencilState  = &depth_stencil;
    pipeline_info.pDynamicState       = &dynamic_state;
    pipeline_info.layout              = pipeline_layout_;
    pipeline_info.renderPass          = render_pass_;
    pipeline_info.subpass             = 0;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(
            context_->get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }
}
template <typename C>
void renderer<C>::create_wireframe_pipeline() {
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_->get_stage_info(), fragment_shader_->get_stage_info()
    };

    auto binding_description    = vertex::get_binding_descriptions();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32_t>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_LINE;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_NONE;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = msaa_samples_;

    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable   = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments    = &color_blend_attachment;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable  = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp   = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable     = VK_FALSE;

    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount          = 2;
    pipeline_info.pStages             = shader_stages;
    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState   = &multisampling;
    pipeline_info.pColorBlendState    = &color_blending;
    pipeline_info.pDepthStencilState  = &depth_stencil;
    pipeline_info.pDynamicState       = &dynamic_state;
    pipeline_info.layout              = pipeline_layout_;
    pipeline_info.renderPass          = render_pass_;
    pipeline_info.subpass             = 0;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(
            context_->get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &wireframe_pipeline_
        ) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create wireframe pipeline!");
    }
}

template <typename C>
void renderer<C>::create_debug_pipeline() {
    // Используем уже созданные шейдеры
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        debug_vertex_shader_->get_stage_info(), debug_fragment_shader_->get_stage_info()
    };

    // Vertex input state
    auto binding_description    = debug_vertex::get_binding_descriptions();
    auto attribute_descriptions = debug_vertex::get_attribute_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32_t>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    VkDynamicState dynamic_states[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    // Rasterizer state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = msaa_samples_;

    // Color blend state
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable   = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments    = &color_blend_attachment;

    VkPipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable  = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_FALSE;
    depth_stencil.depthCompareOp   = VK_COMPARE_OP_LESS;
    depth_stencil.depthBoundsTestEnable = VK_FALSE;
    depth_stencil.stencilTestEnable     = VK_FALSE;

    // Pipeline layout
    // VkPushConstantRange push_constant_range{};
    // push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // push_constant_range.offset     = 0;
    // push_constant_range.size       = sizeof(debug_push_constant_data);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount         = 1;
    pipeline_layout_info.pSetLayouts            = &uniform_descriptor_set_layout_;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges    = nullptr;

    if (vkCreatePipelineLayout(
            context_->get_device(), &pipeline_layout_info, nullptr, &debug_pipeline_layout_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create debug pipeline layout");
    }

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount          = 2;
    pipeline_info.pStages             = shader_stages;
    pipeline_info.pVertexInputState   = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState      = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState   = &multisampling;
    pipeline_info.pColorBlendState    = &color_blending;
    pipeline_info.pDepthStencilState  = &depth_stencil;
    pipeline_info.pDynamicState       = &dynamic_state;
    pipeline_info.layout              = debug_pipeline_layout_;
    pipeline_info.renderPass          = render_pass_;
    pipeline_info.subpass             = 1;
    pipeline_info.basePipelineHandle  = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(
            context_->get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &debug_pipeline_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create debug pipeline");
    }
}

template <typename C>
void renderer<C>::create_framebuffers() {
    framebuffers_.resize(swapchain_image_views_.size());

    for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
        VkImageView attachments[] = {
            color_image_view_,
            depth_image_view_,
            swapchain_image_views_[i],
        };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass      = render_pass_;
        framebuffer_info.attachmentCount = 3;
        framebuffer_info.pAttachments    = attachments;
        framebuffer_info.width           = swapchain_extent_.width;
        framebuffer_info.height          = swapchain_extent_.height;
        framebuffer_info.layers          = 1;

        if (vkCreateFramebuffer(
                context_->get_device(), &framebuffer_info, nullptr, &framebuffers_[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

template <typename C>
void renderer<C>::create_command_buffers() {
    command_buffers_.resize(framebuffers_.size());

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool        = context_->get_command_pool();
    alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

    if (vkAllocateCommandBuffers(context_->get_device(), &alloc_info, command_buffers_.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

template <typename C>
void renderer<C>::create_sync_objects() {
    // Семафоры создаем по количеству кадров в полете
    image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(swapchain_images_.size());
    // Fences создаем по количеству кадров в полете
    in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
    // Инициализируем массив fences для изображений
    images_in_flight_.assign(swapchain_images_.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Создаем семафоры для каждого кадра в полете
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(
                context_->get_device(), &semaphore_info, nullptr, &image_available_semaphores_[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }

    // Создаем семафоры для каждого изображения swapchain
    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        if (vkCreateSemaphore(
                context_->get_device(), &semaphore_info, nullptr, &render_finished_semaphores_[i]
            ) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }

    // Создаем fences для каждого кадра в полете
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(context_->get_device(), &fence_info, nullptr, &in_flight_fences_[i]) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

template <typename C>
void renderer<C>::create_uniform_buffers() {
    VkDeviceSize buffer_size = sizeof(uniform_buffer_object);
    uniform_buffers_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniform_buffers_[i] = std::make_unique<uniform_buffer>(*context_, buffer_size);
    }
}

template <typename C>
void renderer<C>::create_descriptor_pool() {
    constexpr uint32_t MAX_DESCRIPTOR_SETS  = 512;
    constexpr uint32_t STORAGE_BUFFER_COUNT = 1024;

    std::array pool_sizes = {
        VkDescriptorPoolSize{
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 2 + MAX_FRAMES_IN_FLIGHT)
        },
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, STORAGE_BUFFER_COUNT},
        VkDescriptorPoolSize{
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)  // Shadow map для каждого кадра
        }
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();
    pool_info.maxSets       = MAX_DESCRIPTOR_SETS;

    if (vkCreateDescriptorPool(context_->get_device(), &pool_info, nullptr, &descriptor_pool_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

template <typename C>
void renderer<C>::create_descriptor_sets() {
    // Создаем descriptor sets для uniform buffer (set 0)
    std::vector layouts(MAX_FRAMES_IN_FLIGHT, uniform_descriptor_set_layout_);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts        = layouts.data();

    descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(context_->get_device(), &alloc_info, descriptor_sets_.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo ubo_buffer_info{};
        ubo_buffer_info.buffer = uniform_buffers_[i]->get_buffer();
        ubo_buffer_info.offset = 0;
        ubo_buffer_info.range  = sizeof(uniform_buffer_object);

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet          = descriptor_sets_[i];
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo     = &ubo_buffer_info;

        vkUpdateDescriptorSets(context_->get_device(), 1, &descriptor_write, 0, nullptr);
    }
}

template <typename C>
void renderer<C>::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io    = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    setup_imgui_style();

    constexpr bool install_callbacks = true;
    ImGui_ImplGlfw_InitForVulkan(window_->get_handle(), install_callbacks);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance                  = context_->get_instance();
    init_info.PhysicalDevice            = context_->get_physical_device();
    init_info.Device                    = context_->get_device();
    init_info.QueueFamily               = context_->get_queue_families().graphics_family.value();
    init_info.Queue                     = context_->get_graphics_queue();
    init_info.DescriptorPool            = imgui_descriptor_pool_;
    init_info.RenderPass                = render_pass_;
    init_info.Subpass                   = 2;
    init_info.MinImageCount             = 2;
    init_info.ImageCount                = swapchain_images_.size();
    init_info.MSAASamples               = msaa_samples_;
    init_info.Allocator                 = nullptr;
    init_info.CheckVkResultFn           = nullptr;

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        throw std::runtime_error("failed to initialize imgui");
    }
}

template <typename C>
void renderer<C>::setup_imgui_style() {
    // ImGuiStyle& style = ImGui::GetStyle();
    // TODO: setup imgui style
}

template <typename C>
void renderer<C>::create_imgui_descriptor_pool() {
    std::array pool_sizes = {
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();
    pool_info.maxSets       = 1000;

    if (vkCreateDescriptorPool(
            context_->get_device(), &pool_info, nullptr, &imgui_descriptor_pool_
        ) != VK_SUCCESS) {
        throw std::runtime_error("failed to create imgui descriptor pool");
    }
}

template <typename C>
void renderer<C>::cleanup_imgui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (imgui_descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context_->get_device(), imgui_descriptor_pool_, nullptr);
        imgui_descriptor_pool_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_descriptor_pool() {
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context_->get_device(), descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_render_pass() {
    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(context_->get_device(), render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_descriptor_set_layouts() {
    if (storage_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            context_->get_device(), storage_descriptor_set_layout_, nullptr
        );
        storage_descriptor_set_layout_ = VK_NULL_HANDLE;
    }
    if (shadow_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            context_->get_device(), shadow_descriptor_set_layout_, nullptr
        );
        shadow_descriptor_set_layout_ = VK_NULL_HANDLE;
    }
    if (uniform_descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            context_->get_device(), uniform_descriptor_set_layout_, nullptr
        );
        uniform_descriptor_set_layout_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_pipelines() {
    if (graphics_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(context_->get_device(), graphics_pipeline_, nullptr);
        graphics_pipeline_ = VK_NULL_HANDLE;
    }
    if (wireframe_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(context_->get_device(), wireframe_pipeline_, nullptr);
        wireframe_pipeline_ = VK_NULL_HANDLE;
    }
    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(context_->get_device(), pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_shadow_pipeline() {
    if (shadow_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(context_->get_device(), shadow_pipeline_, nullptr);
        shadow_pipeline_ = VK_NULL_HANDLE;
    }
    if (shadow_pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(context_->get_device(), shadow_pipeline_layout_, nullptr);
        shadow_pipeline_layout_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_debug_pipeline() {
    if (debug_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(context_->get_device(), debug_pipeline_, nullptr);
        debug_pipeline_ = VK_NULL_HANDLE;
    }
    if (debug_pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(context_->get_device(), debug_pipeline_layout_, nullptr);
        debug_pipeline_layout_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_swapchain() {
    for (auto* framebuffer : framebuffers_) {
        vkDestroyFramebuffer(context_->get_device(), framebuffer, nullptr);
    }

    for (auto* image_view : swapchain_image_views_) {
        vkDestroyImageView(context_->get_device(), image_view, nullptr);
    }

    vkDestroySwapchainKHR(context_->get_device(), swapchain_, nullptr);

    // Очищаем семафоры при пересоздании swapchain
    for (auto* semaphore : image_available_semaphores_) {
        vkDestroySemaphore(context_->get_device(), semaphore, nullptr);
    }
    for (auto* semaphore : render_finished_semaphores_) {
        vkDestroySemaphore(context_->get_device(), semaphore, nullptr);
    }

    // Очищаем fences при пересоздании swapchain
    for (auto* fence : in_flight_fences_) {
        vkDestroyFence(context_->get_device(), fence, nullptr);
    }
}

template <typename C>
void renderer<C>::cleanup_color_resources() {
    if (color_image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(context_->get_device(), color_image_view_, nullptr);
        color_image_view_ = VK_NULL_HANDLE;
    }
    if (color_image_ != VK_NULL_HANDLE) {
        vkDestroyImage(context_->get_device(), color_image_, nullptr);
        color_image_ = VK_NULL_HANDLE;
    }
    if (color_image_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->get_device(), color_image_memory_, nullptr);
        color_image_memory_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::cleanup_depth_resources() {
    if (depth_image_view_ != VK_NULL_HANDLE) {
        vkDestroyImageView(context_->get_device(), depth_image_view_, nullptr);
        depth_image_view_ = VK_NULL_HANDLE;
    }
    if (depth_image_ != VK_NULL_HANDLE) {
        vkDestroyImage(context_->get_device(), depth_image_, nullptr);
        depth_image_ = VK_NULL_HANDLE;
    }
    if (depth_image_memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->get_device(), depth_image_memory_, nullptr);
        depth_image_memory_ = VK_NULL_HANDLE;
    }
}

template <typename C>
void renderer<C>::recreate_swapchain() {
    int width  = 0;
    int height = 0;
    window_->get_framebuffer_size(&width, &height);
    while (width == 0 || height == 0) {
        window_->get_framebuffer_size(&width, &height);
        window_->poll_events();

        // Добавляем небольшую задержку, чтобы не нагружать CPU
        // когда окно свернуто или имеет нулевой размер
        std::this_thread::sleep_for(std::chrono::milliseconds(16));  // ~60 FPS
    }

    wait_idle();

    cleanup_swapchain();
    cleanup_color_resources();
    cleanup_depth_resources();

    create_swapchain();
    create_image_views();
    create_color_resources();
    create_depth_resources();
    create_framebuffers();
    create_sync_objects();

    current_frame_       = 0;
    current_image_index_ = 0;
}

template <typename WC>
void renderer<WC>::render_world_pass(
    world_type& world, const camera& camera
) {
    stats_.timing.world_pass_uniform_ms = measure_ms([&] {
        update_uniform_buffer(camera);
    });

    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass        = render_pass_;
    render_pass_info.framebuffer       = framebuffers_[current_image_index_];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_extent_;

    VkClearValue clear_values[3]{};
    memcpy(&clear_values[0].color, &clear_color_, sizeof(vec4f));
    clear_values[1].depthStencil = {1.0f, 0};

    render_pass_info.clearValueCount = 3;
    render_pass_info.pClearValues    = clear_values;

    vkCmdBeginRenderPass(
        command_buffers_[current_image_index_], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE
    );

    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapchain_extent_.width);
    viewport.height   = static_cast<float>(swapchain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vkCmdSetViewport(command_buffers_[current_image_index_], 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent_;

    vkCmdSetScissor(command_buffers_[current_image_index_], 0, 1, &scissor);

    stats_.timing.world_pass_geometry_ms = measure_ms([&] {
        render_world(world, camera);
    });

    vkCmdNextSubpass(command_buffers_[current_image_index_], VK_SUBPASS_CONTENTS_INLINE);

    stats_.timing.world_pass_debug_ms = measure_ms([&] {
        render_debug_primitives();
    });

    vkCmdNextSubpass(command_buffers_[current_image_index_], VK_SUBPASS_CONTENTS_INLINE);

    stats_.timing.world_pass_imgui_ms = measure_ms([&] {
        render_imgui();
    });

    vkCmdEndRenderPass(command_buffers_[current_image_index_]);
}

template <typename WC>
void renderer<WC>::render_world(
    world_type& world, const camera& camera
) {
    // Обновить light buffer
    light_buffer_->update(world);

    VkPipeline current_pipeline =
        (current_render_mode_ == render_mode::lit) ? graphics_pipeline_ : wireframe_pipeline_;
    vkCmdBindPipeline(
        command_buffers_[current_image_index_], VK_PIPELINE_BIND_POINT_GRAPHICS, current_pipeline
    );

    // Биндим uniform buffer descriptor set один раз перед циклом
    vkCmdBindDescriptorSets(
        command_buffers_[current_image_index_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_,
        0,
        1,
        &descriptor_sets_[current_frame_],
        0,
        nullptr
    );

    // Биндим shadow map descriptor set (set 2)
    vkCmdBindDescriptorSets(
        command_buffers_[current_image_index_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_,
        2,  // Set index 2 (shadow map descriptor set layout)
        1,
        &shadow_map_descriptor_sets_[current_frame_],
        0,
        nullptr
    );

    // Биндим point lights descriptor set (set 3)
    VkDescriptorSet point_lights_descriptor_set = light_buffer_->get_descriptor_set();
    vkCmdBindDescriptorSets(
        command_buffers_[current_image_index_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_,
        3,  // Set index 3 (point lights descriptor set layout)
        1,
        &point_lights_descriptor_set,
        0,
        nullptr
    );

    // Биндим palette descriptor set (set 4)
    VkDescriptorSet palette_ds = palette_buffer_->get_descriptor_set();
    vkCmdBindDescriptorSets(
        command_buffers_[current_image_index_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline_layout_,
        4,  // Set index 4 (palette descriptor set layout)
        1,
        &palette_ds,
        0,
        nullptr
    );

    const auto& buffers = combined_buffer_pool_->get_buffers();
    for (const auto& buffer : buffers) {
        if (buffer->is_empty()) {
            continue;
        }

        VkBuffer vertex_buffer         = buffer->get_vertex_buffer();
        VkBuffer instance_index_buffer = buffer->get_instance_index_buffer();
        VkBuffer index_buffer          = buffer->get_index_buffer();

        // Биндим storage buffer descriptor set из буфера (set 1)
        VkDescriptorSet buffer_descriptor_set = buffer->get_descriptor_set();
        vkCmdBindDescriptorSets(
            command_buffers_[current_image_index_],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_layout_,
            1,  // Set index 1 (storage buffer descriptor set layout)
            1,
            &buffer_descriptor_set,
            0,
            nullptr
        );

        // Биндим vertex и index буферы
        constexpr VkDeviceSize vertex_offset   = 0;
        constexpr VkDeviceSize instance_offset = 0;

        std::array vertex_buffers = {vertex_buffer, instance_index_buffer};
        std::array vertex_offsets = {vertex_offset, instance_offset};

        vkCmdBindVertexBuffers(
            command_buffers_[current_image_index_],
            0,
            vertex_buffers.size(),
            vertex_buffers.data(),
            vertex_offsets.data()
        );
        vkCmdBindIndexBuffer(
            command_buffers_[current_image_index_], index_buffer, 0, VK_INDEX_TYPE_UINT32
        );

        const uint32_t max_draws = buffer->get_draw_command_count();
        if (max_draws > 0) {
            vkCmdDrawIndexedIndirectCount(
                command_buffers_[current_image_index_],
                buffer->get_culled_indirect_buffer(),
                0,
                buffer->get_count_buffer(),
                0,
                max_draws,
                sizeof(draw_command)
            );
            draw_call_count_++;
        }
    }
}

template <typename C>
void renderer<C>::update_uniform_buffer(
    const camera& camera
) const {
    uniform_buffer_object ubo{};

    // View matrix
    const mat4f& view_matrix = camera.get_view_matrix();
    memcpy(ubo.view, view_matrix.cptr(), sizeof(mat4f));

    // Projection matrix
    const mat4f& projection_matrix = camera.get_projection_matrix();
    memcpy(ubo.projection, projection_matrix.cptr(), sizeof(mat4f));

    // View position
    ubo.view_pos = camera.get_position();

    // Directional light data
    const auto& light_space_matrices = shadow_map_->get_light_space_matrices();
    const auto& cascade_splits       = shadow_map_->get_cascade_splits();
    for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
        ubo.directional_light.light_space_matrices[i] = light_space_matrices[i];
    }
    ubo.directional_light.cascade_splits =
        vec4f{cascade_splits[0], cascade_splits[1], cascade_splits[2], cascade_splits[3]};
    ubo.directional_light.direction = directional_light_settings_.direction;
    ubo.directional_light.color     = directional_light_settings_.color;
    ubo.directional_light.intensity = directional_light_settings_.intensity;

    // Point lights count
    ubo.point_lights_count = light_buffer_->get_lights_count();

    // Fog
    ubo.fog.color         = fog_settings_.color;
    ubo.fog.near_distance = fog_settings_.near_distance;
    ubo.fog.far_distance  = fog_settings_.far_distance;
    ubo.fog.enabled       = fog_settings_.enabled ? 1u : 0u;

    uniform_buffers_[current_frame_]->copy_from_struct(ubo);
}

template <typename C>
void renderer<C>::render_debug_primitives() {
    if (debug_primitives_.is_empty()) {
        return;
    }

    update_debug_vertex_buffer();

    vkCmdBindPipeline(
        command_buffers_[current_image_index_], VK_PIPELINE_BIND_POINT_GRAPHICS, debug_pipeline_
    );

    vkCmdBindDescriptorSets(
        command_buffers_[current_image_index_],
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        debug_pipeline_layout_,
        0,
        1,
        &descriptor_sets_[current_frame_],
        0,
        nullptr
    );

    VkBuffer vertex_buffer        = debug_vertex_buffer_->get_buffer();
    constexpr VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(command_buffers_[current_image_index_], 0, 1, &vertex_buffer, &offset);

    vkCmdDraw(
        command_buffers_[current_image_index_],
        static_cast<uint32>(debug_primitives_.get_vertices().size()),
        1,
        0,
        0
    );
}

template <typename C>
void renderer<C>::update_debug_vertex_buffer() {
    const auto& debug_vertices = debug_primitives_.get_vertices();

    const VkDeviceSize required_size = sizeof(debug_vertex) * debug_vertices.size();
    if (required_size > debug_vertex_buffer_->get_size()) {
        debug_vertex_buffer_ = std::make_unique<vertex_buffer>(*context_, required_size);
    }

    debug_vertex_buffer_->copy_from_vector(debug_vertices);
}

template <typename C>
void renderer<C>::render_imgui() const {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffers_[current_image_index_]);
}

template <typename C>
void* renderer<C>::get_shadow_map_texture_id(
    uint32 cascade_index
) const {
    struct Cache {
        std::array<VkDescriptorSet, shadow_map::cascade_count> sets{};
        std::array<VkImageView, shadow_map::cascade_count> views{};
        VkSampler sampler = VK_NULL_HANDLE;
        bool valid        = false;
    };

    static Cache cache;

    VkSampler current_sampler = shadow_map_->get_debug_sampler();
    bool need_rebuild         = !cache.valid || cache.sampler != current_sampler;

    for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
        VkImageView v = shadow_map_->get_image_view(i);
        if (!cache.valid || cache.views[i] != v) {
            need_rebuild = true;
        }
    }

    if (need_rebuild) {
        cache.sampler = current_sampler;
        for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
            cache.views[i] = shadow_map_->get_image_view(i);
            cache.sets[i]  = ImGui_ImplVulkan_AddTexture(
                cache.sampler, cache.views[i], VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
            );
        }
        cache.valid = true;
    }

    return cache.sets[cascade_index];
}

template <typename C>
VkFormat renderer<C>::find_depth_format() {
    return find_supported_format(
        {
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT,
        },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

template <typename C>
VkFormat renderer<C>::find_supported_format(
    const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features
) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(context_->get_physical_device(), format, &props);
        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == VK_IMAGE_TILING_OPTIMAL &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format for physical device");
}

template <typename C>
void renderer<C>::create_image(
    VkImage& image,
    VkDeviceMemory& image_memory,
    VkExtent2D extent,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkSampleCountFlagBits samples
) {
    VkImageCreateInfo image_info{};
    image_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType     = VK_IMAGE_TYPE_2D;
    image_info.extent.width  = extent.width;
    image_info.extent.height = extent.height;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.format        = format;
    image_info.tiling        = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage         = usage;
    image_info.samples       = samples;
    image_info.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(context_->get_device(), &image_info, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image!");
    }

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(context_->get_device(), image, &mem_requirements);

    VkMemoryAllocateInfo alloc_info{};
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(context_->get_device(), &alloc_info, nullptr, &image_memory) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate image memory!");
    }

    vkBindImageMemory(context_->get_device(), image, image_memory, 0);
}

template <typename C>
VkImageView renderer<C>::create_image_view(
    VkImage image, VkFormat format, VkImageAspectFlags aspect_flags
) {
    VkImageViewCreateInfo view_info{};
    view_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image                           = image;
    view_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format                          = format;
    view_info.subresourceRange.aspectMask     = aspect_flags;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    VkImageView image_view;
    if (vkCreateImageView(context_->get_device(), &view_info, nullptr, &image_view) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image view!");
    }

    return image_view;
}

template <typename C>
uint32 renderer<C>::find_memory_type(
    uint32 typeFilter, VkMemoryPropertyFlags properties
) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(context_->get_physical_device(), &mem_properties);

    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

template <typename C>
VkSurfaceFormatKHR renderer<C>::choose_swap_surface_format(
    const std::vector<VkSurfaceFormatKHR>& available_formats
) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    return available_formats[0];
}

template <typename C>
VkPresentModeKHR renderer<C>::choose_swap_present_mode(
    const std::vector<VkPresentModeKHR>& available_present_modes
) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

template <typename C>
VkExtent2D renderer<C>::choose_swap_extent(
    const VkSurfaceCapabilitiesKHR& capabilities
) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    window_->get_framebuffer_size(&width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actual_extent.width = std::clamp(
        actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
    );
    actual_extent.height = std::clamp(
        actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
    );

    return actual_extent;
}

template <typename C>
auto renderer<C>::get_max_usable_sample_count() -> VkSampleCountFlagBits {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(context_->get_physical_device(), &props);

    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts &
                                props.limits.framebufferDepthSampleCounts;

    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}

}  // namespace vw::gfx

#endif  // VW_GFX_RENDER_RENDERER_INL_H
