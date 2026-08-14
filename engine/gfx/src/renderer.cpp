module;

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

module vw.gfx;

import std;
import vulkan;
import vw.core;
import vw.ecs;
import vw.world;
import vw.platform;
import :vk;

namespace vw::gfx {

renderer::renderer(
    vulkan_context& context, window& window, const block_registry& registry
)
    : context_(&context)
    , window_(&window)
    , block_registry_(&registry)
    , mesh_pool_(context, registry) {
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

    constexpr vk::DeviceSize initial_size = 512 * 2 * sizeof(debug_vertex);
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
        *context_,
        deletion_queue_,
        descriptor_pool_,
        storage_descriptor_set_layout_,
        cull_pipeline_->get_buffer_descriptor_set_layout()
    );

    light_buffer_ = std::make_unique<light_buffer_type>(
        *context_, descriptor_pool_, point_lights_descriptor_set_layout_
    );

    palette_buffer_ = std::make_unique<palette_buffer>(
        *context_, descriptor_pool_, palette_descriptor_set_layout_, *block_registry_
    );
}

renderer::~renderer() {
    mesh_pool_.stop_gen_threads();
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

void renderer::begin_frame() {
    // Ждем завершения предыдущего кадра
    const vk::Device device = context_->get_device();
    vk_must(
        device.waitForFences(in_flight_fences_[current_frame_], vk::True, std::numeric_limits<uint64>::max()),
        "wait for frame fence"
    );

    // Сбрасываем fence для рендеринга перед использованием
    vk_must(device.resetFences(in_flight_fences_[current_frame_]), "reset frame fence");

    // The fence just waited on was signalled by frame_counter_ - MAX_FRAMES_IN_FLIGHT,
    // so anything retired on that frame or earlier is no longer read by the GPU.
    if (frame_counter_ >= MAX_FRAMES_IN_FLIGHT) {
        deletion_queue_.collect(frame_counter_ - MAX_FRAMES_IN_FLIGHT);
    }
    deletion_queue_.set_frame(frame_counter_);

    // Получаем следующий image из swapchain (используем семафор)
    uint32 image_index = 0;
    const vk::Result result = device.acquireNextImageKHR(
        swapchain_,
        std::numeric_limits<uint64>::max(),
        image_available_semaphores_[current_frame_],
        nullptr,
        &image_index
    );

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreate_swapchain();
        return;
    }
    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    current_image_index_ = image_index;

    // Ждем завершения предыдущего использования этого изображения
    if (images_in_flight_[image_index] != nullptr) {
        vk_must(
            device.waitForFences(images_in_flight_[image_index], vk::True, std::numeric_limits<uint64>::max()),
            "wait for image fence"
        );
    }

    // Связываем fence с изображением
    images_in_flight_[image_index] = in_flight_fences_[current_frame_];

    // Подготовка нового кадра ImGui
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void renderer::end_frame() {
    vk::SubmitInfo submit_info{};

    vk::Semaphore wait_semaphores[]      = {image_available_semaphores_[current_frame_]};
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submit_info.waitSemaphoreCount     = 1;
    submit_info.pWaitSemaphores        = wait_semaphores;
    submit_info.pWaitDstStageMask      = wait_stages;
    submit_info.commandBufferCount     = 1;
    submit_info.pCommandBuffers        = &command_buffers_[current_image_index_];

    vk::Semaphore signal_semaphores[]  = {render_finished_semaphores_[current_image_index_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores    = signal_semaphores;

    vk_must(
        context_->get_graphics_queue().submit(submit_info, in_flight_fences_[current_frame_]),
        "submit draw command buffers"
    );

    vk::PresentInfoKHR present_info{};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;

    vk::SwapchainKHR swapchains[] = {swapchain_};
    present_info.swapchainCount = 1;
    present_info.pSwapchains    = swapchains;
    present_info.pImageIndices  = &current_image_index_;

    const vk::Result result = context_->get_present_queue().presentKHR(&present_info);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebuffer_resized_) {
        framebuffer_resized_ = false;
        recreate_swapchain();
    } else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    debug_primitives_.clear();

    stats_.draw_call_count = draw_call_count_;
    draw_call_count_       = 0;

    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
    ++frame_counter_;
}

auto renderer::get_present_mode_name() const -> std::string_view {
    switch (present_mode_) {
        case vk::PresentModeKHR::eImmediate: return "immediate";
        case vk::PresentModeKHR::eMailbox: return "mailbox";
        case vk::PresentModeKHR::eFifo: return "fifo";
        case vk::PresentModeKHR::eFifoRelaxed: return "fifo_relaxed";
        default: return "other";
    }
}

const renderer_stats& renderer::get_stats() const {
    stats_.combined_buffers = combined_buffer_pool_->get_stats();
    return stats_;
}

auto renderer::get_directional_light_settings() -> directional_light_settings& {
    return directional_light_settings_;
}

auto renderer::get_fog_settings() -> fog_settings& {
    return fog_settings_;
}

void renderer::draw_colliders(
    world& w, color col
) {
    for (auto [ent, box, tc] :
         w.registry().view<box_collider_component, transform_component>()) {
        auto pos  = tc.get_position() + box.get_offset();
        auto half = box.get_extents() * 0.5f;
        draw_box(pos - half, box.get_extents(), col);
    }
}

void renderer::set_clear_color(
    float r, float g, float b, float a
) {
    clear_color_ = {r, g, b, a};
}

void renderer::set_clear_color(
    vec4f color
) {
    clear_color_ = color;
}

void renderer::wait_idle() const {
    vk_must(context_->get_device().waitIdle(), "wait for device idle");
}

void renderer::handle_resize() {
    framebuffer_resized_ = true;
}

void renderer::set_render_mode(
    render_mode mode
) {
    current_render_mode_ = mode;
}

render_mode renderer::get_render_mode() const {
    return current_render_mode_;
}

void renderer::sync_meshes_(world_type& world) {
    mesh_pool_.process_completed();

    auto& registry = world.registry();

    for (auto it = pending_mesh_entities_.begin(); it != pending_mesh_entities_.end();) {
        auto ent = *it;
        if (!registry.has<model_component>(ent)) {
            it = pending_mesh_entities_.erase(it);
            continue;
        }
        auto& comp = registry.get<model_component>(ent);
        if (!comp.has_model()) {
            it = pending_mesh_entities_.erase(it);
            continue;
        }
        if (mesh_pool_.has(comp.get_identity())) {
            it = pending_mesh_entities_.erase(it);
            registry.notify_changed<model_component>(ent);
        } else {
            ++it;
        }
    }

    for (auto ent : registry.changed<model_component>()) {
        if (!registry.has<model_component>(ent)) continue;
        auto& comp = registry.get<model_component>(ent);
        if (!comp.has_model()) continue;
        auto identity = comp.get_identity();
        if (!mesh_pool_.has(identity) && !mesh_pool_.is_pending(identity)) {
            mesh_pool_.request_mesh(
                comp.get_model(), mesh_options{.enable_top_brightness = comp.top_brightness()}
            );
            pending_mesh_entities_.insert(ent);
        }
    }
}

void renderer::render(
    world_type& world, camera& camera
) {
    // Начинаем запись в command buffer
    vk::CommandBufferBeginInfo begin_info{};
    begin_info.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    vk_must(command_buffers_[current_image_index_].begin(begin_info), "begin recording command buffer");

    sync_meshes_(world);

    stats_.timing.shadow_map_update_ms =
        measure_ms([&] { shadow_map_->update(camera, directional_light_settings_.direction); });

    const auto& cascade_frustums = shadow_map_->get_cascade_frustums();

    stats_.timing.buffer_pool_update_ms = measure_ms([&] {
        combined_buffer_pool_->update(
            world, camera, command_buffers_[current_image_index_], mesh_pool_);
    });

    stats_.timing.compute_cull_ms = measure_ms([&] {
        {
            vk::MemoryBarrier barrier{};
            barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            barrier.dstAccessMask =                    //
                vk::AccessFlagBits::eVertexAttributeRead |  //
                vk::AccessFlagBits::eIndexRead |             //
                vk::AccessFlagBits::eIndirectCommandRead |  //
                vk::AccessFlagBits::eShaderRead;
            constexpr auto stage_mask =                //
                vk::PipelineStageFlagBits::eVertexInput |   //
                vk::PipelineStageFlagBits::eDrawIndirect |  //
                vk::PipelineStageFlagBits::eVertexShader |  //
                vk::PipelineStageFlagBits::eComputeShader;
            command_buffers_[current_image_index_].pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                stage_mask,
                {},
                barrier,
                nullptr,
                nullptr
            );
        }

        const vw::spatial::frustum& view_frustum = camera.get_frustum();
        cull_pipeline_->update_frustums(current_frame_, view_frustum, cascade_frustums);

        for (const auto& buffer : combined_buffer_pool_->get_buffers()) {
            if (!buffer->is_empty()) {
                cull_pipeline_->dispatch(
                    command_buffers_[current_image_index_], *buffer, current_frame_
                );
            }
        }

        {
            vk::MemoryBarrier compute_barrier{};
            compute_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
            compute_barrier.dstAccessMask = vk::AccessFlagBits::eIndirectCommandRead;
            command_buffers_[current_image_index_].pipelineBarrier(
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eDrawIndirect,
                {},
                compute_barrier,
                nullptr,
                nullptr
            );
        }
    });

    stats_.timing.shadow_pass_ms = measure_ms([&] { render_shadow_pass(world, camera); });

    stats_.timing.world_pass_ms = measure_ms([&] { render_world_pass(world, camera); });

    vk_must(command_buffers_[current_image_index_].end(), "record command buffer");
}

void renderer::draw_line(
    const vec3f& a, const vec3f& b, color col
) {
    debug_primitives_.add_line(a, b, col);
}

void renderer::draw_box(
    const mat4f& matrix, const vec3f& size, const color col
) {
    debug_primitives_.add_box(matrix, size, col);
}

void renderer::draw_box(
    const transform& transform, const vec3f& size, color col
) {
    debug_primitives_.add_box(transform, size, col);
}

void renderer::draw_box(
    const vec3f& position, const vec3f& size, color col
) {
    debug_primitives_.add_box(position, size, col);
}

void renderer::draw_grid(
    const mat4f& matrix, float cell_size, int cols, int rows, color clr
) {
    debug_primitives_.add_grid(matrix, cell_size, cols, rows, clr);
}

void renderer::draw_grid(
    const transform& transform, float cell_size, int cols, int rows, color clr
) {
    debug_primitives_.add_grid(transform, cell_size, cols, rows, clr);
}

void renderer::draw_grid(
    const vec3f& position, float cell_size, int cols, int rows, color clr
) {
    debug_primitives_.add_grid(position, cell_size, cols, rows, clr);
}

void renderer::create_swapchain() {
    auto swapchain_support = context_->query_swapchain_support_();

    vk::SurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    vk::Extent2D extent                 = choose_swap_extent(swapchain_support.capabilities);

    present_mode_ = choose_swap_present_mode(swapchain_support.present_modes);

    uint32 image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 &&
        image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR create_info{};
    create_info.surface          = context_->get_surface();
    create_info.minImageCount    = image_count;
    create_info.imageFormat      = surface_format.format;
    create_info.imageColorSpace  = surface_format.colorSpace;
    create_info.imageExtent      = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;
    create_info.imageSharingMode = vk::SharingMode::eExclusive;
    create_info.preTransform     = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode      = present_mode_;
    create_info.clipped          = vk::True;
    create_info.oldSwapchain     = nullptr;

    auto queue_families = context_->get_queue_families();
    if (queue_families.graphics_family != queue_families.present_family) {
        uint32 queue_family_indices[] = {
            queue_families.graphics_family.value(), queue_families.present_family.value()
        };
        create_info.imageSharingMode      = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices   = queue_family_indices;
    }

    swapchain_ = vk_must(context_->get_device().createSwapchainKHR(create_info), "failed to create swap chain");

    swapchain_images_ =
        vk_must(context_->get_device().getSwapchainImagesKHR(swapchain_), "get swapchain images");

    swapchain_image_format_ = surface_format.format;
    swapchain_extent_       = extent;
}

void renderer::create_image_views() {
    swapchain_image_views_.resize(swapchain_images_.size());

    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        vk::ImageViewCreateInfo view_info{};
        view_info.image                           = swapchain_images_[i];
        view_info.viewType                        = vk::ImageViewType::e2D;
        view_info.format                          = swapchain_image_format_;
        view_info.components.r                    = vk::ComponentSwizzle::eIdentity;
        view_info.components.g                    = vk::ComponentSwizzle::eIdentity;
        view_info.components.b                    = vk::ComponentSwizzle::eIdentity;
        view_info.components.a                    = vk::ComponentSwizzle::eIdentity;
        view_info.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
        view_info.subresourceRange.baseMipLevel   = 0;
        view_info.subresourceRange.levelCount     = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount     = 1;

        swapchain_image_views_[i] = vk_must(context_->get_device().createImageView(view_info), "failed to create image views");
    }
}

void renderer::create_color_resources() {
    create_image(
        color_image_,
        color_image_memory_,
        swapchain_extent_,
        swapchain_image_format_,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        msaa_samples_
    );
    color_image_view_ =
        create_image_view(color_image_, swapchain_image_format_, vk::ImageAspectFlagBits::eColor);
}

void renderer::create_depth_resources() {
    vk::Format depth_format = find_depth_format();

    create_image(
        depth_image_,
        depth_image_memory_,
        swapchain_extent_,
        depth_format,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        msaa_samples_
    );
    depth_image_view_ = create_image_view(depth_image_, depth_format, vk::ImageAspectFlagBits::eDepth);
}

void renderer::create_render_pass() {
    // Attachment 0: MSAA color (render target)
    vk::AttachmentDescription color_attachment{};
    color_attachment.format         = swapchain_image_format_;
    color_attachment.samples        = msaa_samples_;
    color_attachment.loadOp         = vk::AttachmentLoadOp::eClear;
    color_attachment.storeOp        = vk::AttachmentStoreOp::eDontCare;
    color_attachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    color_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_attachment.initialLayout  = vk::ImageLayout::eUndefined;
    color_attachment.finalLayout    = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout     = vk::ImageLayout::eColorAttachmentOptimal;

    // Attachment 1: MSAA depth
    vk::AttachmentDescription depth_attachment{};
    depth_attachment.format         = find_depth_format();
    depth_attachment.samples        = msaa_samples_;
    depth_attachment.loadOp         = vk::AttachmentLoadOp::eClear;
    depth_attachment.storeOp        = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depth_attachment.initialLayout  = vk::ImageLayout::eUndefined;
    depth_attachment.finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depth_attachment_ref{};
    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    // Attachment 2: resolve target (swapchain image, 1x)
    vk::AttachmentDescription color_resolve_attachment{};
    color_resolve_attachment.format         = swapchain_image_format_;
    color_resolve_attachment.samples        = vk::SampleCountFlagBits::e1;
    color_resolve_attachment.loadOp         = vk::AttachmentLoadOp::eDontCare;
    color_resolve_attachment.storeOp        = vk::AttachmentStoreOp::eStore;
    color_resolve_attachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
    color_resolve_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    color_resolve_attachment.initialLayout  = vk::ImageLayout::eUndefined;
    color_resolve_attachment.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference color_resolve_ref{};
    color_resolve_ref.attachment = 2;
    color_resolve_ref.layout     = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass_3d    = {};
    subpass_3d.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass_3d.colorAttachmentCount    = 1;
    subpass_3d.pColorAttachments       = &color_attachment_ref;
    subpass_3d.pDepthStencilAttachment = &depth_attachment_ref;
    subpass_3d.pResolveAttachments     = nullptr;

    vk::SubpassDescription subpass_debug    = {};
    subpass_debug.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass_debug.colorAttachmentCount    = 1;
    subpass_debug.pColorAttachments       = &color_attachment_ref;
    subpass_debug.pDepthStencilAttachment = &depth_attachment_ref;
    subpass_debug.pResolveAttachments     = nullptr;

    vk::SubpassDescription subpass_imgui    = {};
    subpass_imgui.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass_imgui.colorAttachmentCount    = 1;
    subpass_imgui.pColorAttachments       = &color_attachment_ref;
    subpass_imgui.pDepthStencilAttachment = nullptr;
    subpass_imgui.pResolveAttachments     = &color_resolve_ref;

    vk::SubpassDescription subpasses[] = {subpass_3d, subpass_debug, subpass_imgui};

    vk::SubpassDependency dependency_3d = {};
    dependency_3d.srcSubpass          = vk::SubpassExternal;
    dependency_3d.dstSubpass          = 0;
    dependency_3d.srcStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency_3d.srcAccessMask       = {};
    dependency_3d.dstStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency_3d.dstAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::SubpassDependency dependency_debug = {};
    dependency_debug.srcSubpass          = 0;
    dependency_debug.dstSubpass          = 1;
    dependency_debug.srcStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency_debug.srcAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite;
    dependency_debug.dstStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency_debug.dstAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::SubpassDependency dependency_imgui = {};
    dependency_imgui.srcSubpass          = 1;
    dependency_imgui.dstSubpass          = 2;
    dependency_imgui.srcStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency_imgui.srcAccessMask       = vk::AccessFlagBits::eColorAttachmentWrite;
    dependency_imgui.dstStageMask        = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency_imgui.dstAccessMask =
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

    vk::SubpassDependency dependencies[] = {dependency_3d, dependency_debug, dependency_imgui};

    vk::AttachmentDescription attachments[] = {
        color_attachment, depth_attachment, color_resolve_attachment
    };

    vk::RenderPassCreateInfo render_pass_info{};
    render_pass_info.attachmentCount = 3;
    render_pass_info.pAttachments    = attachments;
    render_pass_info.subpassCount    = 3;
    render_pass_info.pSubpasses      = subpasses;
    render_pass_info.dependencyCount = 3;
    render_pass_info.pDependencies   = dependencies;

    render_pass_ = vk_must(context_->get_device().createRenderPass(render_pass_info), "failed to create render pass");
}

void renderer::create_descriptor_set_layouts() {
    // Uniform buffer descriptor set layout (set 0, binding 0)
    vk::DescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding         = 0;
    ubo_layout_binding.descriptorType  = vk::DescriptorType::eUniformBuffer;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags      = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo ubo_layout_info{};
    ubo_layout_info.bindingCount = 1;
    ubo_layout_info.pBindings    = &ubo_layout_binding;

    uniform_descriptor_set_layout_ = vk_must(context_->get_device().createDescriptorSetLayout(ubo_layout_info), "failed to create uniform descriptor set layout");

    // Storage buffer descriptor set layout (set 1: binding 0 = vw::asset::model matrices, binding 1 = normal
    // matrices)
    std::array<vk::DescriptorSetLayoutBinding, 2> storage_layout_bindings{};
    storage_layout_bindings[0].binding            = 0;
    storage_layout_bindings[0].descriptorType     = vk::DescriptorType::eStorageBuffer;
    storage_layout_bindings[0].descriptorCount    = 1;
    storage_layout_bindings[0].stageFlags         = vk::ShaderStageFlagBits::eVertex;
    storage_layout_bindings[0].pImmutableSamplers = nullptr;

    storage_layout_bindings[1].binding            = 1;
    storage_layout_bindings[1].descriptorType     = vk::DescriptorType::eStorageBuffer;
    storage_layout_bindings[1].descriptorCount    = 1;
    storage_layout_bindings[1].stageFlags         = vk::ShaderStageFlagBits::eVertex;
    storage_layout_bindings[1].pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo storage_layout_info{};
    storage_layout_info.bindingCount = storage_layout_bindings.size();
    storage_layout_info.pBindings    = storage_layout_bindings.data();

    storage_descriptor_set_layout_ = vk_must(context_->get_device().createDescriptorSetLayout(storage_layout_info), "failed to create storage descriptor set layout");

    // Shadow map descriptor set layout (set 2, binding 0)
    vk::DescriptorSetLayoutBinding shadow_layout_binding{};
    shadow_layout_binding.binding            = 0;
    shadow_layout_binding.descriptorType     = vk::DescriptorType::eCombinedImageSampler;
    shadow_layout_binding.descriptorCount    = 1;
    shadow_layout_binding.stageFlags         = vk::ShaderStageFlagBits::eFragment;
    shadow_layout_binding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo shadow_layout_info{};
    shadow_layout_info.bindingCount = 1;
    shadow_layout_info.pBindings    = &shadow_layout_binding;

    shadow_descriptor_set_layout_ = vk_must(context_->get_device().createDescriptorSetLayout(shadow_layout_info), "failed to create shadow descriptor set layout");
}

void renderer::create_graphics_pipeline() {
    // Используем уже созданные шейдеры
    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_->get_stage_info(), fragment_shader_->get_stage_info()
    };

    // Vertex input state
    auto binding_description    = vertex::get_binding_descriptions();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input assembly state
    vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = vk::False;

    // Viewport state
    vk::PipelineViewportStateCreateInfo viewport_state{};
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    vk::DynamicState dynamic_states[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    // Rasterizer state
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable        = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode             = vk::PolygonMode::eFill;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable         = vk::False;

    // Multisampling state
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable  = vk::False;
    multisampling.rasterizationSamples = msaa_samples_;

    // Color blend state
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo color_blending{};
    color_blending.logicOpEnable   = vk::False;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments    = &color_blend_attachment;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.depthTestEnable  = vk::True;
    depth_stencil.depthWriteEnable = vk::True;
    depth_stencil.depthCompareOp   = vk::CompareOp::eLess;
    depth_stencil.depthBoundsTestEnable = vk::False;
    depth_stencil.stencilTestEnable     = vk::False;

    std::array<vk::DescriptorSetLayout, 5> descriptor_set_layouts = {
        uniform_descriptor_set_layout_,
        storage_descriptor_set_layout_,
        shadow_descriptor_set_layout_,
        point_lights_descriptor_set_layout_,
        palette_descriptor_set_layout_
    };

    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.setLayoutCount = static_cast<uint32>(descriptor_set_layouts.size());
    pipeline_layout_info.pSetLayouts    = descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges    = nullptr;

    pipeline_layout_ = vk_must(context_->get_device().createPipelineLayout(pipeline_layout_info), "failed to create pipeline layout");

    // Graphics pipeline
    vk::GraphicsPipelineCreateInfo pipeline_info{};

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
    pipeline_info.basePipelineHandle  = nullptr;

    graphics_pipeline_ =
        vk_must(context_->get_device().createGraphicsPipeline(nullptr, pipeline_info), "create graphics pipeline");
}
void renderer::create_wireframe_pipeline() {
    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_->get_stage_info(), fragment_shader_->get_stage_info()
    };

    auto binding_description    = vertex::get_binding_descriptions();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = vk::False;

    // Viewport state
    vk::PipelineViewportStateCreateInfo viewport_state{};
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    vk::DynamicState dynamic_states[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable        = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode             = vk::PolygonMode::eLine;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = vk::CullModeFlagBits::eNone;
    rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable         = vk::False;

    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable  = vk::False;
    multisampling.rasterizationSamples = msaa_samples_;

    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo color_blending{};
    color_blending.logicOpEnable   = vk::False;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments    = &color_blend_attachment;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.depthTestEnable  = vk::True;
    depth_stencil.depthWriteEnable = vk::True;
    depth_stencil.depthCompareOp   = vk::CompareOp::eLess;
    depth_stencil.depthBoundsTestEnable = vk::False;
    depth_stencil.stencilTestEnable     = vk::False;

    vk::GraphicsPipelineCreateInfo pipeline_info{};
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
    pipeline_info.basePipelineHandle  = nullptr;

    wireframe_pipeline_ =
        vk_must(context_->get_device().createGraphicsPipeline(nullptr, pipeline_info), "create wireframe pipeline");
}

void renderer::create_debug_pipeline() {
    // Используем уже созданные шейдеры
    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        debug_vertex_shader_->get_stage_info(), debug_fragment_shader_->get_stage_info()
    };

    // Vertex input state
    auto binding_description    = debug_vertex::get_binding_descriptions();
    auto attribute_descriptions = debug_vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input assembly state
    vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.topology = vk::PrimitiveTopology::eLineList;
    input_assembly.primitiveRestartEnable = vk::False;

    // Viewport state
    vk::PipelineViewportStateCreateInfo viewport_state{};
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    vk::DynamicState dynamic_states[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    // Rasterizer state
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable        = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode             = vk::PolygonMode::eFill;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable         = vk::False;

    // Multisampling state
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable  = vk::False;
    multisampling.rasterizationSamples = msaa_samples_;

    // Color blend state
    vk::PipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    color_blend_attachment.blendEnable = vk::False;

    vk::PipelineColorBlendStateCreateInfo color_blending{};
    color_blending.logicOpEnable   = vk::False;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments    = &color_blend_attachment;

    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.depthTestEnable  = vk::True;
    depth_stencil.depthWriteEnable = vk::False;
    depth_stencil.depthCompareOp   = vk::CompareOp::eLess;
    depth_stencil.depthBoundsTestEnable = vk::False;
    depth_stencil.stencilTestEnable     = vk::False;

    // Pipeline layout
    // vk::PushConstantRange push_constant_range{};
    // push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;
    // push_constant_range.offset     = 0;
    // push_constant_range.size       = sizeof(debug_push_constant_data);

    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.setLayoutCount         = 1;
    pipeline_layout_info.pSetLayouts            = &uniform_descriptor_set_layout_;
    pipeline_layout_info.pushConstantRangeCount = 0;
    pipeline_layout_info.pPushConstantRanges    = nullptr;

    debug_pipeline_layout_ = vk_must(context_->get_device().createPipelineLayout(pipeline_layout_info), "failed to create debug pipeline layout");

    // Graphics pipeline
    vk::GraphicsPipelineCreateInfo pipeline_info{};
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
    pipeline_info.basePipelineHandle  = nullptr;

    debug_pipeline_ =
        vk_must(context_->get_device().createGraphicsPipeline(nullptr, pipeline_info), "create debug pipeline");
}

void renderer::create_framebuffers() {
    framebuffers_.resize(swapchain_image_views_.size());

    for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
        vk::ImageView attachments[] = {
            color_image_view_,
            depth_image_view_,
            swapchain_image_views_[i],
        };

        vk::FramebufferCreateInfo framebuffer_info{};
        framebuffer_info.renderPass      = render_pass_;
        framebuffer_info.attachmentCount = 3;
        framebuffer_info.pAttachments    = attachments;
        framebuffer_info.width           = swapchain_extent_.width;
        framebuffer_info.height          = swapchain_extent_.height;
        framebuffer_info.layers          = 1;

        framebuffers_[i] = vk_must(context_->get_device().createFramebuffer(framebuffer_info), "failed to create framebuffer");
    }
}

void renderer::create_command_buffers() {
    command_buffers_.resize(framebuffers_.size());

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.commandPool        = context_->get_command_pool();
    alloc_info.level              = vk::CommandBufferLevel::ePrimary;
    alloc_info.commandBufferCount = static_cast<uint32>(command_buffers_.size());

    command_buffers_ =
        vk_must(context_->get_device().allocateCommandBuffers(alloc_info), "allocate command buffers");
}

void renderer::create_sync_objects() {
    // Семафоры создаем по количеству кадров в полете
    image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(swapchain_images_.size());
    // Fences создаем по количеству кадров в полете
    in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
    // Инициализируем массив fences для изображений
    images_in_flight_.assign(swapchain_images_.size(), nullptr);

    vk::SemaphoreCreateInfo semaphore_info{};

    vk::FenceCreateInfo fence_info{};
    fence_info.flags = vk::FenceCreateFlagBits::eSignaled;

    // Создаем семафоры для каждого кадра в полете
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        image_available_semaphores_[i] = vk_must(context_->get_device().createSemaphore(semaphore_info), "failed to create synchronization objects for a frame");
    }

    // Создаем семафоры для каждого изображения swapchain
    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        render_finished_semaphores_[i] = vk_must(context_->get_device().createSemaphore(semaphore_info), "failed to create synchronization objects for a frame");
    }

    // Создаем fences для каждого кадра в полете
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        in_flight_fences_[i] = vk_must(context_->get_device().createFence(fence_info), "failed to create synchronization objects for a frame");
    }
}

void renderer::create_uniform_buffers() {
    vk::DeviceSize buffer_size = sizeof(uniform_buffer_object);
    uniform_buffers_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniform_buffers_[i] = std::make_unique<uniform_buffer>(*context_, buffer_size);
    }
}

void renderer::create_descriptor_pool() {
    constexpr uint32 MAX_DESCRIPTOR_SETS  = 512;
    constexpr uint32 STORAGE_BUFFER_COUNT = 1024;

    std::array pool_sizes = {
        vk::DescriptorPoolSize{
            vk::DescriptorType::eUniformBuffer,
            static_cast<uint32>(MAX_FRAMES_IN_FLIGHT * 2 + MAX_FRAMES_IN_FLIGHT)
        },
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, STORAGE_BUFFER_COUNT},
        vk::DescriptorPoolSize{
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32>(MAX_FRAMES_IN_FLIGHT)  // Shadow map для каждого кадра
        }
    };

    vk::DescriptorPoolCreateInfo pool_info{};
    pool_info.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    pool_info.poolSizeCount = static_cast<uint32>(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();
    pool_info.maxSets       = MAX_DESCRIPTOR_SETS;

    descriptor_pool_ = vk_must(context_->get_device().createDescriptorPool(pool_info), "failed to create descriptor pool");
}

void renderer::create_descriptor_sets() {
    // Создаем descriptor sets для uniform buffer (set 0)
    std::vector layouts(MAX_FRAMES_IN_FLIGHT, uniform_descriptor_set_layout_);
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts        = layouts.data();

    descriptor_sets_ =
        vk_must(context_->get_device().allocateDescriptorSets(alloc_info), "allocate descriptor sets");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo ubo_buffer_info{};
        ubo_buffer_info.buffer = uniform_buffers_[i]->get_buffer();
        ubo_buffer_info.offset = 0;
        ubo_buffer_info.range  = sizeof(uniform_buffer_object);

        vk::WriteDescriptorSet descriptor_write{};
        descriptor_write.dstSet          = descriptor_sets_[i];
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = vk::DescriptorType::eUniformBuffer;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo     = &ubo_buffer_info;

        context_->get_device().updateDescriptorSets(descriptor_write, nullptr);
    }
}

void renderer::init_imgui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io    = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    setup_imgui_style();

    constexpr bool install_callbacks = true;
    ImGui_ImplGlfw_InitForVulkan(
        static_cast<GLFWwindow*>(window_->native_handle()), install_callbacks);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.ApiVersion                = VK_API_VERSION_1_2;
    init_info.Instance                  = context_->get_instance();
    init_info.PhysicalDevice            = context_->get_physical_device();
    init_info.Device                    = context_->get_device();
    init_info.QueueFamily               = context_->get_queue_families().graphics_family.value();
    init_info.Queue                     = context_->get_graphics_queue();
    init_info.DescriptorPool            = imgui_descriptor_pool_;
    init_info.MinImageCount             = 2;
    init_info.ImageCount                = swapchain_images_.size();
    init_info.Allocator                 = nullptr;
    init_info.CheckVkResultFn           = nullptr;

    init_info.PipelineInfoMain.RenderPass  = render_pass_;
    init_info.PipelineInfoMain.Subpass     = 2;
    init_info.PipelineInfoMain.MSAASamples = static_cast<VkSampleCountFlagBits>(msaa_samples_);

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        throw std::runtime_error("failed to initialize imgui");
    }
}

void renderer::setup_imgui_style() {
    // ImGuiStyle& style = ImGui::GetStyle();
    // TODO: setup imgui style
}

void renderer::create_imgui_descriptor_pool() {
    std::array pool_sizes = {
        vk::DescriptorPoolSize{vk::DescriptorType::eSampler, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eSampledImage, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageImage, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformTexelBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageTexelBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBuffer, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBufferDynamic, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eStorageBufferDynamic, 1000},
        vk::DescriptorPoolSize{vk::DescriptorType::eInputAttachment, 1000}
    };

    vk::DescriptorPoolCreateInfo pool_info{};
    pool_info.flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    pool_info.poolSizeCount = static_cast<uint32>(pool_sizes.size());
    pool_info.pPoolSizes    = pool_sizes.data();
    pool_info.maxSets       = 1000;

    imgui_descriptor_pool_ = vk_must(context_->get_device().createDescriptorPool(pool_info), "failed to create imgui descriptor pool");
}

void renderer::cleanup_imgui() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (imgui_descriptor_pool_ != nullptr) {
        context_->get_device().destroyDescriptorPool(imgui_descriptor_pool_);
        imgui_descriptor_pool_ = nullptr;
    }
}

void renderer::cleanup_descriptor_pool() {
    if (descriptor_pool_ != nullptr) {
        context_->get_device().destroyDescriptorPool(descriptor_pool_);
        descriptor_pool_ = nullptr;
    }
}

void renderer::cleanup_render_pass() {
    if (render_pass_ != nullptr) {
        context_->get_device().destroyRenderPass(render_pass_);
        render_pass_ = nullptr;
    }
}

void renderer::cleanup_descriptor_set_layouts() {
    if (storage_descriptor_set_layout_ != nullptr) {
        context_->get_device().destroyDescriptorSetLayout(storage_descriptor_set_layout_);
        storage_descriptor_set_layout_ = nullptr;
    }
    if (shadow_descriptor_set_layout_ != nullptr) {
        context_->get_device().destroyDescriptorSetLayout(shadow_descriptor_set_layout_);
        shadow_descriptor_set_layout_ = nullptr;
    }
    if (uniform_descriptor_set_layout_ != nullptr) {
        context_->get_device().destroyDescriptorSetLayout(uniform_descriptor_set_layout_);
        uniform_descriptor_set_layout_ = nullptr;
    }
}

void renderer::cleanup_pipelines() {
    if (graphics_pipeline_ != nullptr) {
        context_->get_device().destroyPipeline(graphics_pipeline_);
        graphics_pipeline_ = nullptr;
    }
    if (wireframe_pipeline_ != nullptr) {
        context_->get_device().destroyPipeline(wireframe_pipeline_);
        wireframe_pipeline_ = nullptr;
    }
    if (pipeline_layout_ != nullptr) {
        context_->get_device().destroyPipelineLayout(pipeline_layout_);
        pipeline_layout_ = nullptr;
    }
}

void renderer::cleanup_shadow_pipeline() {
    if (shadow_pipeline_ != nullptr) {
        context_->get_device().destroyPipeline(shadow_pipeline_);
        shadow_pipeline_ = nullptr;
    }
    if (shadow_pipeline_layout_ != nullptr) {
        context_->get_device().destroyPipelineLayout(shadow_pipeline_layout_);
        shadow_pipeline_layout_ = nullptr;
    }
}

void renderer::cleanup_debug_pipeline() {
    if (debug_pipeline_ != nullptr) {
        context_->get_device().destroyPipeline(debug_pipeline_);
        debug_pipeline_ = nullptr;
    }
    if (debug_pipeline_layout_ != nullptr) {
        context_->get_device().destroyPipelineLayout(debug_pipeline_layout_);
        debug_pipeline_layout_ = nullptr;
    }
}

void renderer::cleanup_swapchain() {
    for (auto framebuffer : framebuffers_) {
        context_->get_device().destroyFramebuffer(framebuffer);
    }

    for (auto image_view : swapchain_image_views_) {
        context_->get_device().destroyImageView(image_view);
    }

    context_->get_device().destroySwapchainKHR(swapchain_);

    // Очищаем семафоры при пересоздании swapchain
    for (auto semaphore : image_available_semaphores_) {
        context_->get_device().destroySemaphore(semaphore);
    }
    for (auto semaphore : render_finished_semaphores_) {
        context_->get_device().destroySemaphore(semaphore);
    }

    // Очищаем fences при пересоздании swapchain
    for (auto fence : in_flight_fences_) {
        context_->get_device().destroyFence(fence);
    }
}

void renderer::cleanup_color_resources() {
    if (color_image_view_ != nullptr) {
        context_->get_device().destroyImageView(color_image_view_);
        color_image_view_ = nullptr;
    }
    if (color_image_ != nullptr) {
        context_->get_device().destroyImage(color_image_);
        color_image_ = nullptr;
    }
    if (color_image_memory_ != nullptr) {
        context_->get_device().freeMemory(color_image_memory_);
        color_image_memory_ = nullptr;
    }
}

void renderer::cleanup_depth_resources() {
    if (depth_image_view_ != nullptr) {
        context_->get_device().destroyImageView(depth_image_view_);
        depth_image_view_ = nullptr;
    }
    if (depth_image_ != nullptr) {
        context_->get_device().destroyImage(depth_image_);
        depth_image_ = nullptr;
    }
    if (depth_image_memory_ != nullptr) {
        context_->get_device().freeMemory(depth_image_memory_);
        depth_image_memory_ = nullptr;
    }
}

void renderer::recreate_swapchain() {
    vec2i size = window_->framebuffer_size();
    while (size.x == 0 || size.y == 0) {
        size = window_->framebuffer_size();
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

void renderer::render_world_pass(
    world_type& world, const camera& camera
) {
    stats_.timing.world_pass_uniform_ms = measure_ms([&] { update_uniform_buffer(camera); });

    vk::RenderPassBeginInfo render_pass_info{};
    render_pass_info.renderPass        = render_pass_;
    render_pass_info.framebuffer       = framebuffers_[current_image_index_];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_extent_;

    vk::ClearValue clear_values[3]{};
    memcpy(&clear_values[0].color, &clear_color_, sizeof(vec4f));
    clear_values[1].depthStencil = {1.0f, 0};

    render_pass_info.clearValueCount = 3;
    render_pass_info.pClearValues    = clear_values;

    command_buffers_[current_image_index_].beginRenderPass(render_pass_info, vk::SubpassContents::eInline);

    vk::Viewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapchain_extent_.width);
    viewport.height   = static_cast<float>(swapchain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    command_buffers_[current_image_index_].setViewport(0, viewport);

    vk::Rect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent_;

    command_buffers_[current_image_index_].setScissor(0, scissor);

    stats_.timing.world_pass_geometry_ms = measure_ms([&] { render_world(world, camera); });

    command_buffers_[current_image_index_].nextSubpass(vk::SubpassContents::eInline);

    stats_.timing.world_pass_debug_ms = measure_ms([&] { render_debug_primitives(); });

    command_buffers_[current_image_index_].nextSubpass(vk::SubpassContents::eInline);

    stats_.timing.world_pass_imgui_ms = measure_ms([&] { render_imgui(); });

    command_buffers_[current_image_index_].endRenderPass();
}

void renderer::render_world(
    world_type& world, const camera& camera
) {
    // Обновить light buffer
    light_buffer_->update(world);

    vk::Pipeline current_pipeline =
        (current_render_mode_ == render_mode::lit) ? graphics_pipeline_ : wireframe_pipeline_;
    command_buffers_[current_image_index_].bindPipeline(vk::PipelineBindPoint::eGraphics, current_pipeline);

    // Биндим uniform buffer descriptor set один раз перед циклом
    command_buffers_[current_image_index_].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipeline_layout_,
        0,
        descriptor_sets_[current_frame_],
        nullptr
    );

    // Биндим shadow map descriptor set (set 2)
    command_buffers_[current_image_index_].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        pipeline_layout_,
        2,  // Set index 2 (shadow map descriptor set layout)
        1,
        &shadow_map_descriptor_sets_[current_frame_],
        0,
        nullptr);

    // Биндим point lights descriptor set (set 3)
    vk::DescriptorSet point_lights_descriptor_set = light_buffer_->get_descriptor_set();
    command_buffers_[current_image_index_].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        pipeline_layout_,
        3,  // Set index 3 (point lights descriptor set layout)
        1,
        &point_lights_descriptor_set,
        0,
        nullptr);

    // Биндим palette descriptor set (set 4)
    vk::DescriptorSet palette_ds = palette_buffer_->get_descriptor_set();
    command_buffers_[current_image_index_].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        pipeline_layout_,
        4,  // Set index 4 (palette descriptor set layout)
        1,
        &palette_ds,
        0,
        nullptr);

    const auto& buffers = combined_buffer_pool_->get_buffers();
    for (const auto& buffer : buffers) {
        if (buffer->is_empty()) {
            continue;
        }

        vk::Buffer vertex_buffer         = buffer->get_vertex_buffer();
        vk::Buffer instance_index_buffer = buffer->get_instance_index_buffer();
        vk::Buffer index_buffer          = buffer->get_index_buffer();

        // Биндим storage buffer descriptor set из буфера (set 1)
        vk::DescriptorSet buffer_descriptor_set = buffer->get_descriptor_set();
        command_buffers_[current_image_index_].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            pipeline_layout_,
            1,  // Set index 1 (storage buffer descriptor set layout)
            1,
            &buffer_descriptor_set,
            0,
            nullptr);

        // Биндим vertex и index буферы
        constexpr vk::DeviceSize vertex_offset   = 0;
        constexpr vk::DeviceSize instance_offset = 0;

        std::array vertex_buffers = {vertex_buffer, instance_index_buffer};
        std::array vertex_offsets = {vertex_offset, instance_offset};

        command_buffers_[current_image_index_].bindVertexBuffers(0, vertex_buffers, vertex_offsets);
        command_buffers_[current_image_index_].bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint32);

        const uint32 max_draws = buffer->get_draw_command_count();
        if (max_draws > 0) {
            command_buffers_[current_image_index_].drawIndexedIndirectCount(buffer->get_culled_indirect_buffer(),
                0,
                buffer->get_count_buffer(),
                0,
                max_draws,
                sizeof(draw_command));
            draw_call_count_++;
        }
    }
}

void renderer::update_uniform_buffer(
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

void renderer::render_debug_primitives() {
    if (debug_primitives_.is_empty()) {
        return;
    }

    update_debug_vertex_buffer();

    command_buffers_[current_image_index_].bindPipeline(vk::PipelineBindPoint::eGraphics, debug_pipeline_);

    command_buffers_[current_image_index_].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        debug_pipeline_layout_,
        0,
        descriptor_sets_[current_frame_],
        nullptr
    );

    vk::Buffer vertex_buffer        = debug_vertex_buffer_->get_buffer();
    constexpr vk::DeviceSize offset = 0;
    command_buffers_[current_image_index_].bindVertexBuffers(0, vertex_buffer, offset);

    command_buffers_[current_image_index_].draw(static_cast<uint32>(debug_primitives_.get_vertices().size()),
        1,
        0,
        0);
}

void renderer::update_debug_vertex_buffer() {
    const auto& debug_vertices = debug_primitives_.get_vertices();

    const vk::DeviceSize required_size = sizeof(debug_vertex) * debug_vertices.size();
    if (required_size > debug_vertex_buffer_->get_size()) {
        debug_vertex_buffer_ = std::make_unique<vertex_buffer>(*context_, required_size);
    }

    debug_vertex_buffer_->copy_from_vector(debug_vertices);
}

void renderer::render_imgui() const {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffers_[current_image_index_]);
}

void* renderer::get_shadow_map_texture_id(
    uint32 cascade_index
) const {
    struct Cache {
        std::array<vk::DescriptorSet, shadow_map::cascade_count> sets{};
        std::array<vk::ImageView, shadow_map::cascade_count> views{};
        vk::Sampler sampler = nullptr;
        bool valid        = false;
    };

    static Cache cache;

    vk::Sampler current_sampler = shadow_map_->get_debug_sampler();
    bool need_rebuild         = !cache.valid || cache.sampler != current_sampler;

    for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
        vk::ImageView v = shadow_map_->get_image_view(i);
        if (!cache.valid || cache.views[i] != v) {
            need_rebuild = true;
        }
    }

    if (need_rebuild) {
        cache.sampler = current_sampler;
        for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
            cache.views[i] = shadow_map_->get_image_view(i);
            cache.sets[i]  = ImGui_ImplVulkan_AddTexture(
                cache.sampler,
                cache.views[i],
                static_cast<VkImageLayout>(vk::ImageLayout::eDepthStencilReadOnlyOptimal)
            );
        }
        cache.valid = true;
    }

    return cache.sets[cascade_index];
}

vk::Format renderer::find_depth_format() {
    return find_supported_format(
        {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint,
        },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::Format renderer::find_supported_format(
    const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features
) {
    for (vk::Format format : candidates) {
        const vk::FormatProperties props =
            context_->get_physical_device().getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal &&
            (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format for physical device");
}

void renderer::create_image(
    vk::Image& image,
    vk::DeviceMemory& image_memory,
    vk::Extent2D extent,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::SampleCountFlagBits samples
) {
    vk::ImageCreateInfo image_info{};
    image_info.imageType     = vk::ImageType::e2D;
    image_info.extent.width  = extent.width;
    image_info.extent.height = extent.height;
    image_info.extent.depth  = 1;
    image_info.mipLevels     = 1;
    image_info.arrayLayers   = 1;
    image_info.format        = format;
    image_info.tiling        = tiling;
    image_info.initialLayout = vk::ImageLayout::eUndefined;
    image_info.usage         = usage;
    image_info.samples       = samples;
    image_info.sharingMode   = vk::SharingMode::eExclusive;

    image = vk_must(context_->get_device().createImage(image_info), "failed to create image");

    const vk::MemoryRequirements mem_requirements =
        context_->get_device().getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo alloc_info{};
    alloc_info.allocationSize  = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

    image_memory = vk_must(context_->get_device().allocateMemory(alloc_info), "failed to allocate image memory");

    vk_must(
        context_->get_device().bindImageMemory(image, image_memory, 0), "bind image memory"
    );
}

vk::ImageView renderer::create_image_view(
    vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags
) {
    vk::ImageViewCreateInfo view_info{};
    view_info.image                           = image;
    view_info.viewType                        = vk::ImageViewType::e2D;
    view_info.format                          = format;
    view_info.subresourceRange.aspectMask     = aspect_flags;
    view_info.subresourceRange.baseMipLevel   = 0;
    view_info.subresourceRange.levelCount     = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount     = 1;

    vk::ImageView image_view;
    image_view = vk_must(context_->get_device().createImageView(view_info), "failed to create image view");

    return image_view;
}

uint32 renderer::find_memory_type(
    uint32 typeFilter, vk::MemoryPropertyFlags properties
) {
    const vk::PhysicalDeviceMemoryProperties mem_properties =
        context_->get_physical_device().getMemoryProperties();

    for (uint32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type");
}

vk::SurfaceFormatKHR renderer::choose_swap_surface_format(
    const std::vector<vk::SurfaceFormatKHR>& available_formats
) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb &&
            available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }
    return available_formats[0];
}

vk::PresentModeKHR renderer::choose_swap_present_mode(
    const std::vector<vk::PresentModeKHR>& available_present_modes
) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D renderer::choose_swap_extent(
    const vk::SurfaceCapabilitiesKHR& capabilities
) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max()) {
        return capabilities.currentExtent;
    }

    const vec2i size = window_->framebuffer_size();

    vk::Extent2D actual_extent = {static_cast<uint32>(size.x), static_cast<uint32>(size.y)};

    actual_extent.width = std::clamp(
        actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width
    );
    actual_extent.height = std::clamp(
        actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height
    );

    return actual_extent;
}

auto renderer::get_max_usable_sample_count() const -> vk::SampleCountFlagBits {
    const vk::PhysicalDeviceProperties props = context_->get_physical_device().getProperties();

    const vk::SampleCountFlags counts =                //
        props.limits.framebufferColorSampleCounts &  //
        props.limits.framebufferDepthSampleCounts;

    if (counts & vk::SampleCountFlagBits::e4)
        return vk::SampleCountFlagBits::e4;
    if (counts & vk::SampleCountFlagBits::e2)
        return vk::SampleCountFlagBits::e2;
    return vk::SampleCountFlagBits::e1;
}

}  // namespace vw::gfx

namespace vw::gfx {

void renderer::create_palette_descriptor_set_layout() {
    vk::DescriptorSetLayoutBinding palette_layout_binding{};
    palette_layout_binding.binding            = 0;
    palette_layout_binding.descriptorType     = vk::DescriptorType::eStorageBuffer;
    palette_layout_binding.descriptorCount    = 1;
    palette_layout_binding.stageFlags         = vk::ShaderStageFlagBits::eVertex;
    palette_layout_binding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo palette_layout_info{};
    palette_layout_info.bindingCount = 1;
    palette_layout_info.pBindings    = &palette_layout_binding;

    palette_descriptor_set_layout_ = vk_must(context_->get_device().createDescriptorSetLayout(palette_layout_info), "failed to create palette descriptor set layout");
}

void renderer::cleanup_palette_resources() {
    if (palette_descriptor_set_layout_ != nullptr) {
        context_->get_device().destroyDescriptorSetLayout(palette_descriptor_set_layout_);
        palette_descriptor_set_layout_ = nullptr;
    }
}

}  // namespace vw::gfx

namespace vw::gfx {

void renderer::create_point_lights_descriptor_set_layout() {
    // Point lights storage buffer descriptor set layout (set 3, binding 0)
    vk::DescriptorSetLayoutBinding point_lights_layout_binding{};
    point_lights_layout_binding.binding            = 0;
    point_lights_layout_binding.descriptorType     = vk::DescriptorType::eStorageBuffer;
    point_lights_layout_binding.descriptorCount    = 1;
    point_lights_layout_binding.stageFlags         = vk::ShaderStageFlagBits::eFragment;
    point_lights_layout_binding.pImmutableSamplers = nullptr;

    vk::DescriptorSetLayoutCreateInfo point_lights_layout_info{};
    point_lights_layout_info.bindingCount = 1;
    point_lights_layout_info.pBindings    = &point_lights_layout_binding;

    point_lights_descriptor_set_layout_ = vk_must(context_->get_device().createDescriptorSetLayout(point_lights_layout_info), "failed to create point lights descriptor set layout");
}

void renderer::cleanup_point_lights_resources() {
    if (point_lights_descriptor_set_layout_ != nullptr) {
        context_->get_device().destroyDescriptorSetLayout(point_lights_descriptor_set_layout_);
        point_lights_descriptor_set_layout_ = nullptr;
    }
}

}  // namespace vw::gfx

namespace vw::gfx {

void renderer::create_shadow_uniform_buffers() {
    vk::DeviceSize buffer_size = sizeof(shadow_uniform_buffer_object);
    shadow_uniform_buffers_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        shadow_uniform_buffers_[i] = std::make_unique<uniform_buffer>(*context_, buffer_size);
    }
}

void renderer::create_shadow_descriptor_sets() {
    std::vector layouts(MAX_FRAMES_IN_FLIGHT, uniform_descriptor_set_layout_);
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts        = layouts.data();

    shadow_descriptor_sets_ =
        vk_must(context_->get_device().allocateDescriptorSets(alloc_info), "allocate shadow descriptor sets");

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo ubo_buffer_info{};
        ubo_buffer_info.buffer = shadow_uniform_buffers_[i]->get_buffer();
        ubo_buffer_info.offset = 0;
        ubo_buffer_info.range  = sizeof(shadow_uniform_buffer_object);

        vk::WriteDescriptorSet descriptor_write{};
        descriptor_write.dstSet          = shadow_descriptor_sets_[i];
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = vk::DescriptorType::eUniformBuffer;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo     = &ubo_buffer_info;

        context_->get_device().updateDescriptorSets(descriptor_write, nullptr);
    }
}

void renderer::create_shadow_map_descriptor_sets() {
    std::vector layouts(MAX_FRAMES_IN_FLIGHT, shadow_descriptor_set_layout_);
    vk::DescriptorSetAllocateInfo alloc_info{};
    alloc_info.descriptorPool     = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts        = layouts.data();

    shadow_map_descriptor_sets_ =
        vk_must(context_->get_device().allocateDescriptorSets(alloc_info), "allocate shadow map descriptor sets");

    // Обновляем descriptor sets с shadow map array image view и sampler
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorImageInfo image_info{};
        image_info.imageLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
        image_info.imageView   = shadow_map_->get_array_image_view();
        image_info.sampler     = shadow_map_->get_sampler();

        vk::WriteDescriptorSet descriptor_write{};
        descriptor_write.dstSet          = shadow_map_descriptor_sets_[i];
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pImageInfo      = &image_info;

        context_->get_device().updateDescriptorSets(descriptor_write, nullptr);
    }
}

void renderer::create_shadow_pipeline() {
    // Используем shadow шейдеры
    vk::PipelineShaderStageCreateInfo shader_stages[] = {
        shadow_vertex_shader_->get_stage_info(), shadow_fragment_shader_->get_stage_info()
    };

    // Vertex input state (только позиция)
    auto binding_description    = vertex::get_binding_descriptions();
    auto attribute_descriptions = vertex::get_attribute_descriptions();

    vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.vertexBindingDescriptionCount =
        static_cast<uint32>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<uint32>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input assembly state
    vk::PipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
    input_assembly.primitiveRestartEnable = vk::False;

    // Viewport state
    vk::PipelineViewportStateCreateInfo viewport_state{};
    viewport_state.viewportCount = 1;
    viewport_state.pViewports    = nullptr;
    viewport_state.scissorCount  = 1;
    viewport_state.pScissors     = nullptr;

    // Dynamic state
    vk::DynamicState dynamic_states[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.dynamicStateCount = 2;
    dynamic_state.pDynamicStates    = dynamic_states;

    // Rasterizer state (для shadow mapping включаем depth bias)
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.depthClampEnable        = vk::False;
    rasterizer.rasterizerDiscardEnable = vk::False;
    rasterizer.polygonMode             = vk::PolygonMode::eFill;
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = vk::CullModeFlagBits::eFront;
    rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable         = vk::True;
    rasterizer.depthBiasConstantFactor = 1.25f;
    rasterizer.depthBiasSlopeFactor    = 1.75f;
    rasterizer.depthBiasClamp          = 0.0f;

    // Multisampling state
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sampleShadingEnable  = vk::False;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    // Color blend state (нет color attachments для shadow pass)
    vk::PipelineColorBlendStateCreateInfo color_blending{};
    color_blending.logicOpEnable   = vk::False;
    color_blending.attachmentCount = 0;
    color_blending.pAttachments    = nullptr;

    // Depth stencil state
    vk::PipelineDepthStencilStateCreateInfo depth_stencil{};
    depth_stencil.depthTestEnable  = vk::True;
    depth_stencil.depthWriteEnable = vk::True;
    depth_stencil.depthCompareOp   = vk::CompareOp::eLess;
    depth_stencil.depthBoundsTestEnable = vk::False;
    depth_stencil.stencilTestEnable     = vk::False;

    vk::PushConstantRange push_constant_range{};
    push_constant_range.offset     = 0;
    push_constant_range.size       = sizeof(shadow_push_constant_data);
    push_constant_range.stageFlags = vk::ShaderStageFlagBits::eVertex;

    // Pipeline layout для shadow pass (uniform и storage descriptor set layouts)
    std::array shadow_descriptor_set_layouts = {
        uniform_descriptor_set_layout_, storage_descriptor_set_layout_
    };

    vk::PipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.setLayoutCount =
        static_cast<uint32>(shadow_descriptor_set_layouts.size());
    pipeline_layout_info.pSetLayouts            = shadow_descriptor_set_layouts.data();
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges    = &push_constant_range;

    shadow_pipeline_layout_ = vk_must(context_->get_device().createPipelineLayout(pipeline_layout_info), "failed to create shadow pipeline layout");

    // Graphics pipeline для shadow pass
    vk::GraphicsPipelineCreateInfo pipeline_info{};

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
    pipeline_info.layout              = shadow_pipeline_layout_;
    pipeline_info.renderPass          = shadow_map_->get_render_pass();
    pipeline_info.subpass             = 0;
    pipeline_info.basePipelineHandle  = nullptr;

    shadow_pipeline_ =
        vk_must(context_->get_device().createGraphicsPipeline(nullptr, pipeline_info), "create shadow pipeline");
}

void renderer::update_shadow_uniform_buffer() const {
    shadow_uniform_buffer_object ubo{};
    // Directional light data
    const auto& light_space_matrices = shadow_map_->get_light_space_matrices();
    for (uint32 i = 0; i < shadow_map::cascade_count; ++i) {
        ubo.light_space_matrices[i] = light_space_matrices[i];
    }
    shadow_uniform_buffers_[current_frame_]->copy_from_struct(ubo);
}

void renderer::render_shadow_pass(
    world_type& world, const camera& camera
) {
    update_shadow_uniform_buffer();

    // Рендерим каждый каскад отдельно
    for (uint32 cascade_index = 0; cascade_index < shadow_map::cascade_count; ++cascade_index) {
        // Начинаем shadow render pass для текущего каскада
        vk::RenderPassBeginInfo render_pass_info{};
        render_pass_info.renderPass        = shadow_map_->get_render_pass();
        render_pass_info.framebuffer       = shadow_map_->get_framebuffer(cascade_index);
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = {
            shadow_map_->get_size(), shadow_map_->get_size()
        };

        vk::ClearValue clear_value{};
        clear_value.depthStencil = {1.0f, 0};

        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues    = &clear_value;

        command_buffers_[current_image_index_].beginRenderPass(render_pass_info, vk::SubpassContents::eInline);

        // Устанавливаем viewport и scissor для shadow map
        vk::Viewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(shadow_map_->get_size());
        viewport.height   = static_cast<float>(shadow_map_->get_size());
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        command_buffers_[current_image_index_].setViewport(0, viewport);

        vk::Rect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = {shadow_map_->get_size(), shadow_map_->get_size()};
        command_buffers_[current_image_index_].setScissor(0, scissor);

        // Биндим shadow pipeline
        command_buffers_[current_image_index_].bindPipeline(vk::PipelineBindPoint::eGraphics,
            shadow_pipeline_);

        // Биндим shadow uniform buffer descriptor set (set 0)
        command_buffers_[current_image_index_].bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        shadow_pipeline_layout_,
        0,
        shadow_descriptor_sets_[current_frame_],
        nullptr
    );

        // Устанавливаем push constant с индексом каскада
        shadow_push_constant_data push_constants{
            .cascade_index = cascade_index,
        };
        command_buffers_[current_image_index_].pushConstants<shadow_push_constant_data>(shadow_pipeline_layout_, vk::ShaderStageFlagBits::eVertex, 0, push_constants);

        // Рендерим все объекты из combined_buffer_pool
        const auto& buffers = combined_buffer_pool_->get_buffers();
        for (const auto& buffer : buffers) {
            if (buffer->is_empty()) {
                continue;
            }

            vk::Buffer vertex_buffer         = buffer->get_vertex_buffer();
            vk::Buffer instance_index_buffer = buffer->get_instance_index_buffer();
            vk::Buffer index_buffer          = buffer->get_index_buffer();

            // Биндим storage buffer descriptor set из буфера (set 1)
            vk::DescriptorSet buffer_descriptor_set = buffer->get_descriptor_set();
            command_buffers_[current_image_index_].bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                shadow_pipeline_layout_,
                1,  // Set index 1 (storage buffer descriptor set layout)
                1,
                &buffer_descriptor_set,
                0,
                nullptr);

            // Биндим vertex и index буферы
            constexpr vk::DeviceSize vertex_offset   = 0;
            constexpr vk::DeviceSize instance_offset = 0;

            std::array vertex_buffers = {vertex_buffer, instance_index_buffer};
            std::array vertex_offsets = {vertex_offset, instance_offset};

            command_buffers_[current_image_index_].bindVertexBuffers(0,
                vertex_buffers.size(),
                vertex_buffers.data(),
                vertex_offsets.data());
            command_buffers_[current_image_index_].bindIndexBuffer(index_buffer, 0, vk::IndexType::eUint32);

            const uint32 max_draws  = buffer->get_draw_command_count();
            const uint32 pass_index = cascade_index + 1;
            if (max_draws > 0) {
                command_buffers_[current_image_index_].drawIndexedIndirectCount(buffer->get_culled_indirect_buffer(),
                    static_cast<vk::DeviceSize>(pass_index) * max_draws * sizeof(draw_command),
                    buffer->get_count_buffer(),
                    pass_index * sizeof(uint32),
                    max_draws,
                    sizeof(draw_command));
            }
        }

        command_buffers_[current_image_index_].endRenderPass();
    }
}

}  // namespace vw::gfx
