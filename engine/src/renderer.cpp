#include "voxel/renderer.h"
#include "voxel/vulkan_context.h"
#include "voxel/window.h"
#include "voxel/camera.h"
#include "voxel/mesh.h"
#include "voxel/shader.h"
#include "voxel/buffer.h"
#include "voxel/world.h"
#include "voxel/buffer.h"
#include "voxel/math_utils.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <chrono>

namespace voxel {

renderer::renderer(std::shared_ptr<vulkan_context> context, std::shared_ptr<window> window)
    : context_(std::move(context)), window_(std::move(window)), swapchain_(VK_NULL_HANDLE), render_pass_(VK_NULL_HANDLE),
      descriptor_set_layout_(VK_NULL_HANDLE), pipeline_layout_(VK_NULL_HANDLE), graphics_pipeline_(VK_NULL_HANDLE),
      descriptor_pool_(VK_NULL_HANDLE), current_frame_(0),
      current_image_index_(0), framebuffer_resized_(false), images_in_flight_() {
    
    clear_color_ = colorf(0.1f, 0.1f, 0.1f, 1.0f);

    // Создаем шейдеры
    vertex_shader_ = std::make_unique<shader>(context_, "shaders/voxel_vert.spv", shader_type::VERTEX);
    fragment_shader_ = std::make_unique<shader>(context_, "shaders/voxel_frag.spv", shader_type::FRAGMENT);

    create_swapchain();
    create_image_views();
    create_render_pass();
    create_descriptor_set_layout();
    create_graphics_pipeline();
    create_framebuffers();
    create_command_buffers();
    create_sync_objects();
    create_uniform_buffers();
    create_descriptor_pool();
    create_descriptor_sets();
}

renderer::~renderer() {
    wait_idle();
    cleanup_swapchain();
    
    // Освобождаем pipeline
    if (graphics_pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(context_->get_device(), graphics_pipeline_, nullptr);
        graphics_pipeline_ = VK_NULL_HANDLE;
    }
    
    // Освобождаем pipeline layout
    if (pipeline_layout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(context_->get_device(), pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }
    
    // Освобождаем descriptor set layout
    if (descriptor_set_layout_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(context_->get_device(), descriptor_set_layout_, nullptr);
        descriptor_set_layout_ = VK_NULL_HANDLE;
    }
    
    // Освобождаем render pass
    if (render_pass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(context_->get_device(), render_pass_, nullptr);
        render_pass_ = VK_NULL_HANDLE;
    }
    
    // Освобождаем descriptor pool
    if (descriptor_pool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(context_->get_device(), descriptor_pool_, nullptr);
        descriptor_pool_ = VK_NULL_HANDLE;
    }
    
    // Освобождаем шейдеры
    vertex_shader_.reset();
    fragment_shader_.reset();
}

void renderer::begin_frame() {
    // Ждем завершения предыдущего кадра
    vkWaitForFences(context_->get_device(), 1, &in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);

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
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to acquire swap chain image!");
    }

    current_image_index_ = image_index;

    // Ждем завершения предыдущего использования этого изображения
    if (images_in_flight_[image_index] != VK_NULL_HANDLE) {
        vkWaitForFences(context_->get_device(), 1, &images_in_flight_[image_index], VK_TRUE, UINT64_MAX);
    }

    // Связываем fence с изображением
    images_in_flight_[image_index] = in_flight_fences_[current_frame_];
}

void renderer::end_frame() {
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Используем семафор для ожидания получения изображения
    VkSemaphore wait_semaphores[] = {image_available_semaphores_[current_frame_]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers_[current_image_index_];

    VkSemaphore signal_semaphores[] = {render_finished_semaphores_[current_image_index_]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    if (vkQueueSubmit(context_->get_graphics_queue(), 1, &submit_info, in_flight_fences_[current_frame_]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {swapchain_};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &current_image_index_;

    VkResult result = vkQueuePresentKHR(context_->get_present_queue(), &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized_) {
        framebuffer_resized_ = false;
        recreate_swapchain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image!");
    }

    current_frame_ = (current_frame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void renderer::render_mesh(std::shared_ptr<mesh> mesh, const vec3f& position, const vec3f& rotation, const vec3f& scale) {
    if (!mesh) return;
    
    // Биндим меш
    mesh->bind(command_buffers_[current_image_index_]);
    
    // Рисуем меш с индексами
    mesh->draw_indexed(command_buffers_[current_image_index_]);
}

void renderer::render_world(const std::shared_ptr<world>& world, const std::shared_ptr<camera>& camera) {
    if (!camera || !world) return;
    
    // Обновляем uniform buffer для текущего кадра
    update_uniform_buffer(camera);

    // Начинаем запись в command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(command_buffers_[current_image_index_], &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }

    // Начинаем рендер пасс
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_;
    render_pass_info.framebuffer = framebuffers_[current_image_index_];
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain_extent_;

    VkClearValue clear_color = {{{clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffers_[current_image_index_], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    // Биндим pipeline
    vkCmdBindPipeline(command_buffers_[current_image_index_], VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline_);

    // Биндим descriptor set
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

    // Рендерим все объекты в мире
    const auto& objects = world->get_renderable_objects();
    for (const auto& obj : objects) {
        if (obj->visible && obj->pmesh) {
            // Подготавливаем push constant данные с матрицей модели
            push_constant_data push_data{};
            const mat4f& model_matrix = obj->transform.get_matrix();
            for (int i = 0; i < 16; i++) {
                push_data.model[i] = model_matrix[i];
            }
            
            // Отправляем push constants
            vkCmdPushConstants(
                command_buffers_[current_image_index_],
                pipeline_layout_,
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(push_constant_data),
                &push_data
            );
            
            // Биндим меш объекта
            obj->pmesh->bind(command_buffers_[current_image_index_]);
            
            // Рисуем меш с индексами
            obj->pmesh->draw_indexed(command_buffers_[current_image_index_]);
        }
    }

    vkCmdEndRenderPass(command_buffers_[current_image_index_]);

    if (vkEndCommandBuffer(command_buffers_[current_image_index_]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }
}

void renderer::set_clear_color(const colorf& color) {
    clear_color_ = color;
}

void renderer::set_clear_color(float r, float g, float b, float a) {
    clear_color_.r = r;
    clear_color_.g = g;
    clear_color_.b = b;
    clear_color_.a = a;
}

void renderer::wait_idle() {
    vkDeviceWaitIdle(context_->get_device());
}

void renderer::handle_resize() {
    framebuffer_resized_ = true;
}

void renderer::create_swapchain() {
    auto swapchain_support = context_->query_swapchain_support();

    VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes);
    VkExtent2D extent = choose_swap_extent(swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = context_->get_surface();
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    auto queue_families = context_->get_queue_families();
    if (queue_families.graphics_family != queue_families.present_family) {
        uint32_t queue_family_indices[] = {queue_families.graphics_family.value(), queue_families.present_family.value()};
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }

    if (vkCreateSwapchainKHR(context_->get_device(), &create_info, nullptr, &swapchain_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(context_->get_device(), swapchain_, &image_count, nullptr);
    swapchain_images_.resize(image_count);
    vkGetSwapchainImagesKHR(context_->get_device(), swapchain_, &image_count, swapchain_images_.data());

    swapchain_image_format_ = surface_format.format;
    swapchain_extent_ = extent;
}

void renderer::create_image_views() {
    swapchain_image_views_.resize(swapchain_images_.size());

    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = swapchain_images_[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = swapchain_image_format_;
        view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(context_->get_device(), &view_info, nullptr, &swapchain_image_views_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void renderer::create_render_pass() {
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchain_image_format_;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(context_->get_device(), &render_pass_info, nullptr, &render_pass_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }
}

void renderer::create_descriptor_set_layout() {
    VkDescriptorSetLayoutBinding ubo_layout_binding{};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info{};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_layout_binding;

    if (vkCreateDescriptorSetLayout(context_->get_device(), &layout_info, nullptr, &descriptor_set_layout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor set layout!");
    }
}

void renderer::create_graphics_pipeline() {
    // Используем уже созданные шейдеры
    VkPipelineShaderStageCreateInfo shader_stages[] = {
        vertex_shader_->get_stage_info(),
        fragment_shader_->get_stage_info()
    };

    // Vertex input state
    auto binding_description = vertex::get_binding_descriptions();
    auto attribute_descriptions = vertex::get_attribute_descriptions();
    
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = static_cast<uint32_t>(binding_description.size());
    vertex_input_info.pVertexBindingDescriptions = binding_description.data();
    vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    // Input assembly state
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Viewport state
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain_extent_.width);
    viewport.height = static_cast<float>(swapchain_extent_.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_extent_;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    // Rasterizer state
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    // Multisampling state
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blend state
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;

    // Pipeline layout
    VkPushConstantRange push_constant_range{};
    push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_constant_range.offset = 0;
    push_constant_range.size = sizeof(push_constant_data);

    VkPipelineLayoutCreateInfo pipeline_layout_info{};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &descriptor_set_layout_;
    pipeline_layout_info.pushConstantRangeCount = 1;
    pipeline_layout_info.pPushConstantRanges = &push_constant_range;

    if (vkCreatePipelineLayout(context_->get_device(), &pipeline_layout_info, nullptr, &pipeline_layout_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(context_->get_device(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphics_pipeline_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
}

void renderer::create_framebuffers() {
    framebuffers_.resize(swapchain_image_views_.size());

    for (size_t i = 0; i < swapchain_image_views_.size(); i++) {
        VkImageView attachments[] = {
            swapchain_image_views_[i]
        };

        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass_;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments;
        framebuffer_info.width = swapchain_extent_.width;
        framebuffer_info.height = swapchain_extent_.height;
        framebuffer_info.layers = 1;

        if (vkCreateFramebuffer(context_->get_device(), &framebuffer_info, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}

void renderer::create_command_buffers() {
    command_buffers_.resize(framebuffers_.size());

    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = context_->get_command_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers_.size());

    if (vkAllocateCommandBuffers(context_->get_device(), &alloc_info, command_buffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers!");
    }
}

void renderer::create_sync_objects() {
    // Семафоры создаем по количеству кадров в полете
    image_available_semaphores_.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores_.resize(swapchain_images_.size());
    // Fences создаем по количеству кадров в полете
    in_flight_fences_.resize(MAX_FRAMES_IN_FLIGHT);
    // Инициализируем массив fences для изображений
    images_in_flight_.resize(swapchain_images_.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Создаем семафоры для каждого кадра в полете
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(context_->get_device(), &semaphore_info, nullptr, &image_available_semaphores_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }

    // Создаем семафоры для каждого изображения swapchain
    for (size_t i = 0; i < swapchain_images_.size(); i++) {
        if (vkCreateSemaphore(context_->get_device(), &semaphore_info, nullptr, &render_finished_semaphores_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }

    // Создаем fences для каждого кадра в полете
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateFence(context_->get_device(), &fence_info, nullptr, &in_flight_fences_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }
    }
}

void renderer::create_uniform_buffers() {
    VkDeviceSize buffer_size = sizeof(uniform_buffer_object);
    uniform_buffers_.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uniform_buffers_[i] = std::make_unique<uniform_buffer>(context_, buffer_size);
    }
}

void renderer::create_descriptor_pool() {
    VkDescriptorPoolSize pool_size{};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    pool_info.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if (vkCreateDescriptorPool(context_->get_device(), &pool_info, nullptr, &descriptor_pool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create descriptor pool!");
    }
}

void renderer::create_descriptor_sets() {
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptor_set_layout_);
    VkDescriptorSetAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    alloc_info.pSetLayouts = layouts.data();

    descriptor_sets_.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(context_->get_device(), &alloc_info, descriptor_sets_.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffers_[i]->get_buffer();
        buffer_info.offset = 0;
        buffer_info.range = sizeof(uniform_buffer_object);

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet = descriptor_sets_[i];
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;

        vkUpdateDescriptorSets(context_->get_device(), 1, &descriptor_write, 0, nullptr);
    }
}

void renderer::cleanup_swapchain() {
    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(context_->get_device(), framebuffer, nullptr);
    }

    for (auto image_view : swapchain_image_views_) {
        vkDestroyImageView(context_->get_device(), image_view, nullptr);
    }

    vkDestroySwapchainKHR(context_->get_device(), swapchain_, nullptr);

    // Очищаем семафоры при пересоздании swapchain
    for (auto semaphore : image_available_semaphores_) {
        vkDestroySemaphore(context_->get_device(), semaphore, nullptr);
    }
    for (auto semaphore : render_finished_semaphores_) {
        vkDestroySemaphore(context_->get_device(), semaphore, nullptr);
    }

    // Очищаем fences при пересоздании swapchain
    for (auto fence : in_flight_fences_) {
        vkDestroyFence(context_->get_device(), fence, nullptr);
    }
}

void renderer::recreate_swapchain() {
    int width = 0, height = 0;
    window_->get_framebuffer_size(&width, &height);
    while (width == 0 || height == 0) {
        window_->get_framebuffer_size(&width, &height);
        window_->poll_events();
        
        // Добавляем небольшую задержку, чтобы не нагружать CPU
        // когда окно свернуто или имеет нулевой размер
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    wait_idle();

    cleanup_swapchain();

    create_swapchain();
    create_image_views();
    create_framebuffers();
    create_sync_objects(); // Пересоздаем семафоры для нового количества изображений
}

void renderer::update_uniform_buffer(const std::shared_ptr<camera>& camera) {
    uniform_buffer_object ubo{};
    
    // View matrix
    mat4f view_matrix = camera->get_view_matrix();
    for (int i = 0; i < 16; i++) {
        ubo.view[i] = view_matrix[i];
    }
    
    // Projection matrix
    mat4f proj_matrix = camera->get_projection_matrix();
    for (int i = 0; i < 16; i++) {
        ubo.projection[i] = proj_matrix[i];
    }
    
    // View position
    vec3f view_pos = camera->get_position();
    ubo.view_pos = view_pos;
    
    // Light position and color (hardcoded for now)
    ubo.light_pos = vec3f(2.0f, 2.0f, 2.0f);
    ubo.light_color = vec3f(1.0f, 1.0f, 1.0f);

    uniform_buffers_[current_frame_]->copy_from(&ubo, sizeof(ubo));
}

VkSurfaceFormatKHR renderer::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }
    return available_formats[0];
}

VkPresentModeKHR renderer::choose_swap_present_mode(const std::vector<VkPresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D renderer::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        window_->get_framebuffer_size(&width, &height);

        VkExtent2D actual_extent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actual_extent.width = std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actual_extent;
    }
}

} // namespace voxel 
