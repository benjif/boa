#define VMA_IMPLEMENTATION
#include "boa/ecs/ecs.h"
#include "boa/iteration.h"
#include "boa/gfx/renderer.h"
#include "boa/gfx/asset/animation.h"
#include "boa/gfx/vk/initializers.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "imgui.h"
#include "GLFW/glfw3.h"
#include <set>
#include <unordered_map>
#include <fstream>
#include <chrono>
#include <stack>

#define BENCHMARK_FRAME_COUNT 30000

namespace boa::gfx {

Renderer::Renderer()
    : m_asset_manager(*this)
{
#ifndef NDEBUG
    auto boa_start_time = std::chrono::high_resolution_clock::now();
#endif
    init_window_user_pointers();
    init_window();
    create_instance();
    setup_debug_messenger();
    create_surface();
    select_physical_device();
    create_logical_device();
    create_allocator();
    create_swapchain();
    create_default_renderpass();
    create_framebuffer();
    create_commands();
    create_sync_objects();
    create_descriptors();
    create_pipelines();
    create_skybox_resources();
    init_imgui();

#ifndef NDEBUG
    auto boa_end_time = std::chrono::high_resolution_clock::now();
    LOG_INFO("(Renderer) It took {} seconds for Boa's renderer to initialize",
        std::chrono::duration<float, std::chrono::seconds::period>(boa_end_time - boa_start_time).count());
#endif
}

Renderer::~Renderer() {
    cleanup();
}

void Renderer::run() {
#ifdef BENCHMARK
    auto render_start_time = std::chrono::high_resolution_clock::now();
#endif

    auto last_time = std::chrono::high_resolution_clock::now();
    auto current_time = last_time;

    while (!m_window.should_close()) {
        m_window.poll_events();

        current_time = std::chrono::high_resolution_clock::now();
        float time_change
            = std::chrono::duration<float, std::chrono::seconds::period>(current_time - last_time).count();
        last_time = current_time;

        m_per_frame_callback(time_change);
        draw_frame();

#ifdef BENCHMARK
        if (m_frame == BENCHMARK_FRAME_COUNT)
            break;
#endif
    }

#ifdef BENCHMARK
    auto render_stop_time = std::chrono::high_resolution_clock::now();
    float render_time = std::chrono::duration<float, std::chrono::seconds::period>(render_stop_time - render_start_time).count();
    fmt::print("Reached {} frames after {} seconds.\nAverage: {} FPS\n",
        BENCHMARK_FRAME_COUNT, render_time, BENCHMARK_FRAME_COUNT / render_time);
#endif

    m_device.get().waitIdle();
}

void Renderer::set_per_frame_callback(std::function<void(float)> callback) {
    m_per_frame_callback = callback;
}

void Renderer::set_ui_mouse_enabled(bool mouse_enabled) {
    if (mouse_enabled)
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
    else
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
}

void Renderer::init_window() {
    m_window.set_window_user_pointer(&m_user_pointers);
    m_window.set_framebuffer_size_callback(framebuffer_size_callback);
    m_window.set_keyboard_callback(m_keyboard.keyboard_callback);
    m_window.set_cursor_callback(m_mouse.cursor_callback);
    m_window.set_mouse_button_callback(m_mouse.mouse_button_callback);
    m_window.set_mouse_scroll_callback(m_mouse.scroll_callback);
}

void Renderer::init_window_user_pointers() {
    m_user_pointers.renderer = this;
    m_user_pointers.keyboard = &m_keyboard;
    m_user_pointers.mouse = &m_mouse;
}

void Renderer::create_allocator() {
    VmaAllocatorCreateInfo alloc_info{
        .physicalDevice     = m_physical_device,
        .device             = m_device.get(),
        .instance           = m_instance.get(),
        .vulkanApiVersion   = VK_API_VERSION_1_2,
    };

    vmaCreateAllocator(&alloc_info, &m_allocator);
}

void Renderer::framebuffer_size_callback(void *user_ptr_v, int w, int h) {
    //auto user_pointers = reinterpret_cast<Renderer::WindowUserPointers *>(user_ptr_v);
    //auto renderer = user_pointers->renderer;
    // TODO: implement proper framebuffer resizing
}

void Renderer::cleanup() {
    m_deletion_queue.flush();

    if (validation_enabled)
        destroy_debug_utils_messenger_ext(m_instance.get(), m_debug_messenger, nullptr);

    vmaDestroyAllocator(m_allocator);
}

void Renderer::draw_frame() {
    m_device.get().waitForFences(1, &current_frame().render_fence, VK_TRUE, 1e9);
    m_device.get().resetFences(1, &current_frame().render_fence);

    ImGui::Render();

    uint32_t image_index;
    try {
        image_index = m_device.get().acquireNextImageKHR(
            m_swapchain,
            1e9,
            current_frame().present_sem
        ).value;
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    vk::CommandBuffer frame_cmd = current_frame().command_buffer;

    frame_cmd.reset();

    vk::CommandBufferBeginInfo begin_info{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    };

    frame_cmd.begin(begin_info);

    std::array<vk::ClearValue, 2> clear_values{
        vk::ClearColorValue{
            std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f },
        },
        vk::ClearDepthStencilValue{ 1.0f, 0 },
    };

    const vk::ImageView attachments[] = {
        m_msaa_image_view,
        m_depth_image_view,
        m_swapchain_image_views[image_index],
    };

    vk::RenderPassAttachmentBeginInfo attach_begin_info{
        .attachmentCount    = 3,
        .pAttachments       = attachments,
    };

    vk::RenderPassBeginInfo render_pass_info{
        .pNext              = &attach_begin_info,
        .renderPass         = m_renderpass,
        .framebuffer        = m_framebuffer,
        .renderArea         = {
            .offset         = {
                .x          = 0,
                .y          = 0,
            },
            .extent         = m_window_extent,
        },
        .clearValueCount    = static_cast<uint32_t>(clear_values.size()),
        .pClearValues       = clear_values.data(),
    };

    // START DRAW COMMANDS
    frame_cmd.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);

    vk::Viewport viewport{
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = (float)m_window_extent.width,
        .height     = (float)m_window_extent.height,
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    vk::Rect2D scissor{
        .offset = { .x = 0, .y = 0 },
        .extent = m_window_extent,
    };

    frame_cmd.setViewport(0, viewport);
    frame_cmd.setScissor(0, scissor);

    draw_renderables(frame_cmd);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frame_cmd);

    // END DRAW COMMANDS
    frame_cmd.endRenderPass();

    try {
        frame_cmd.end();
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to finalize command buffer");
    }

    vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    vk::SubmitInfo submit_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &current_frame().present_sem,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame_cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &current_frame().render_sem,
    };

    try {
        m_graphics_queue.submit(submit_info, current_frame().render_fence);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to submit to graphics queue");
    }

    vk::PresentInfoKHR present_info{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &current_frame().render_sem,
        .swapchainCount = 1,
        .pSwapchains = &m_swapchain,
        .pImageIndices = &image_index,
        .pResults = nullptr,
    };

    try {
        m_graphics_queue.presentKHR(present_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to present image");
    }

    m_frame++;
}

static const std::array<Vertex, 24> skybox_vertices = {
    Vertex{ .position = { -5, -5,  5 } },
    Vertex{ .position = { -5, -5, -5 } },
    Vertex{ .position = {  5, -5, -5 } },
    Vertex{ .position = {  5, -5,  5 } },
    Vertex{ .position = { -5,  5,  5 } },
    Vertex{ .position = {  5,  5,  5 } },
    Vertex{ .position = {  5,  5, -5 } },
    Vertex{ .position = { -5,  5, -5 } },
    Vertex{ .position = { -5, -5,  5 } },
    Vertex{ .position = {  5, -5,  5 } },
    Vertex{ .position = {  5,  5,  5 } },
    Vertex{ .position = { -5,  5,  5 } },
    Vertex{ .position = {  5, -5,  5 } },
    Vertex{ .position = {  5, -5, -5 } },
    Vertex{ .position = {  5,  5, -5 } },
    Vertex{ .position = {  5,  5,  5 } },
    Vertex{ .position = {  5, -5, -5 } },
    Vertex{ .position = { -5, -5, -5 } },
    Vertex{ .position = { -5,  5, -5 } },
    Vertex{ .position = {  5,  5, -5 } },
    Vertex{ .position = { -5, -5, -5 } },
    Vertex{ .position = { -5, -5,  5 } },
    Vertex{ .position = { -5,  5,  5 } },
    Vertex{ .position = { -5,  5, -5 } },
};

static const std::array<uint32_t, 36> skybox_indices = {
    0,  1,  2,  2,  3,  0,
    4,  5,  6,  6,  7,  4,
    8,  9,  10, 10, 11, 8,
    12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16,
    20, 21, 22, 22, 23, 20,
};

void Renderer::draw_renderables(vk::CommandBuffer cmd) {
    Transformations transforms{
        .view = glm::lookAt(
            m_camera.get_position(),
            m_camera.get_position() + m_camera.get_target(),
            m_camera.get_up()
        ),
        .projection = glm::perspective(
            glm::radians(90.0f),
            m_window_extent.width / (float)m_window_extent.height,
            0.1f,
            500.0f
        )
    };

    transforms.projection[1][1] *= -1;
    transforms.view_projection = transforms.projection * transforms.view;
    transforms.skybox_view_projection = transforms.projection * glm::mat4(glm::mat3(transforms.view));

    void *data;
    vmaMapMemory(m_allocator, current_frame().transformations_buffer.allocation, &data);
    memcpy(data, &transforms, sizeof(Transformations));
    vmaUnmapMemory(m_allocator, current_frame().transformations_buffer.allocation);

    auto &entity_group = ecs::EntityGroup::get();

    BlinnPhong blinn_phong;

    auto global_light = entity_group.find_first_entity_with_component<GlobalLight>();
    if (global_light.has_value()) {
        blinn_phong.global_light = entity_group.get_component<GlobalLight>(global_light.value());
    } else {
        blinn_phong.global_light.direction  = { 0.0f, 0.0f, 0.0f };
        blinn_phong.global_light.ambient    = { 1.0f, 1.0f, 1.0f };
        blinn_phong.global_light.diffuse    = { 1.0f, 1.0f, 1.0f };
        blinn_phong.global_light.specular   = { 1.0f, 1.0f, 1.0f };
    }

    size_t count = 0;
    entity_group.for_each_entity_with_component<PointLight>([&](auto &e_id) {
        if (count >= MAX_POINT_LIGHTS)
            return Iteration::Break;

        blinn_phong.point_lights[count] = entity_group.get_component<PointLight>(e_id);

        count++;
        return Iteration::Continue;
    });
    blinn_phong.point_lights_count = count;
    blinn_phong.camera_position = m_camera.get_position();

    vmaMapMemory(m_allocator, current_frame().blinn_phong_buffer.allocation, &data);
    memcpy(data, &blinn_phong, sizeof(BlinnPhong));
    vmaUnmapMemory(m_allocator, current_frame().blinn_phong_buffer.allocation);

    m_frustum.update(transforms.view_projection);

    // TODO: figure out how to do instanced rendering
    entity_group.for_each_entity_with_component<RenderableModel>([&](auto &e_id) {
        auto &model = entity_group.get_component<RenderableModel>(e_id);

        if (model.nodes.size() == 0)
            return Iteration::Continue;

        bool is_animated = entity_group.has_component<Animated>(e_id);

        const auto draw_node = [&](const auto &node, glm::mat4 local_transform) {
            auto draw_node_impl = [&](const auto &node, glm::mat4 local_transform, auto &draw_node_ref) mutable -> void {
                if (is_animated)
                    local_transform *= entity_group.get_component<Animated>(e_id).transform_for_node(node.id);
                else
                    local_transform *= node.transform_matrix;

                size_t last_material = std::numeric_limits<size_t>::max();
                for (size_t primitive_idx : node.primitives) {
                    const auto &primitive = model.primitives[primitive_idx];

                    // probably not right, it won't matter once we do frustum culling on the GPU
                    // with instanced rendering
                    //glm::vec3 new_bounding_center = local_transform * glm::vec4(primitive.bounding_sphere.center, 1.0f);
                    //double new_bounding_radius = glm::length(local_transform * glm::vec4(0.0f, 0.0f, 0.0f, primitive.bounding_sphere.radius));
                    //if (!m_frustum.is_sphere_within(new_bounding_center, new_bounding_radius))
                        //continue;

                    glm::mat4 model_view_projection = transforms.view_projection * local_transform;
                    PushConstants push_constants = {
                        .extra0 = { -1, -1, -1, -1 },
                        .extra1 = { 0.f, 0.f, 0.f, 0.f },
                        .model_view_projection = model_view_projection
                    };

                    auto &material = m_asset_manager.get_material(primitive.material);
                    if (primitive.material != last_material) {
                        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, material.pipeline);
                        last_material = primitive.material;

                        switch (model.lighting) {
                        case LightingInteractivity::BlinnPhong:
                            cmd.bindDescriptorSets(
                                vk::PipelineBindPoint::eGraphics,
                                material.pipeline_layout,
                                0,
                                current_frame().parent_blinn_phong_set,
                                nullptr);
                            push_constants.model_view_projection = local_transform;
                            break;
                        case LightingInteractivity::Unlit:
                            cmd.bindDescriptorSets(
                                vk::PipelineBindPoint::eGraphics,
                                material.pipeline_layout,
                                0,
                                current_frame().parent_set,
                                nullptr);
                            break;
                        }

                        push_constants.extra0[1] = static_cast<int32_t>(material.color_type);

                        if ((VkDescriptorSet)material.texture_set != VK_NULL_HANDLE && material.color_type == Material::ColorType::Texture) {
                            cmd.bindDescriptorSets(
                                vk::PipelineBindPoint::eGraphics,
                                material.pipeline_layout,
                                1,
                                material.texture_set,
                                nullptr);
                            push_constants.extra0[0] = material.descriptor_number;
                        } else {
                            push_constants.extra0[0] = -1;
                        }
                    }

                    cmd.pushConstants(material.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(PushConstants), &push_constants);

                    vk::Buffer vertex_buffers[] = { model.vertex_buffer.buffer };
                    vk::DeviceSize offsets[] = { 0 };
                    cmd.bindVertexBuffers(0, 1, vertex_buffers, offsets);
                    cmd.bindIndexBuffer(primitive.index_buffer.buffer, 0, vk::IndexType::eUint32);

                    cmd.drawIndexed(primitive.index_count, 1, 0, 0, 0);
                }

                for (size_t child_idx : node.children)
                    draw_node_ref(model.nodes[child_idx], local_transform, draw_node_ref);
            };

            draw_node_impl(node, local_transform, draw_node_impl);
        };

        glm::mat4 entity_transform_matrix{ 1.0f };
        if (entity_group.has_component<Transformable>(e_id))
            entity_transform_matrix = entity_group.get_component<Transformable>(e_id).transform_matrix;

        for (size_t node_idx : model.root_nodes)
            draw_node(model.nodes[node_idx], entity_transform_matrix);

        //Sphere bounding_sphere = Sphere::bounding_sphere_from_bounding_box(model.bounding_box);;
        if (m_draw_bounding_boxes /*&& m_frustum.is_sphere_within(bounding_sphere.center, bounding_sphere.radius)*/) {
            auto &bounding_box_material = m_asset_manager.get_material(BOUNDING_BOX_MATERIAL_INDEX);
            cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, bounding_box_material.pipeline);

            cmd.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                bounding_box_material.pipeline_layout,
                0,
                current_frame().parent_set,
                nullptr);

            vk::Buffer vertex_buffers[] = { model.bounding_box_vertex_buffer.buffer };
            vk::DeviceSize offsets[] = { 0 };
            cmd.bindVertexBuffers(0, 1, vertex_buffers, offsets);

            cmd.draw(24, 1, 0, 0);
        }

        return Iteration::Continue;
    });

    auto skybox_e = m_asset_manager.get_active_skybox();
    if (skybox_e.has_value()) {
        auto &skybox = entity_group.get_component<Skybox>(skybox_e.value());
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_skybox_pipeline);
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_skybox_pipeline_layout,
            0,
            current_frame().parent_set,
            nullptr);
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_skybox_pipeline_layout,
            1,
            skybox.skybox_set,
            nullptr);

        vk::Buffer vertex_buffers[] = { m_skybox_vertex_buffer.buffer };
        vk::DeviceSize offsets[] = { 0 };
        cmd.bindVertexBuffers(0, 1, vertex_buffers, offsets);
        cmd.bindIndexBuffer(m_skybox_index_buffer.buffer, 0, vk::IndexType::eUint32);

        cmd.drawIndexed(skybox_indices.size(), 1, 0, 0, 0);
    }
}

void Renderer::create_skybox_resources() {
    const size_t vertex_buffer_size = skybox_vertices.size() * sizeof(Vertex);
    VmaBuffer staging_buffer = create_buffer(vertex_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(m_allocator, staging_buffer.allocation, &data);
    memcpy(data, skybox_vertices.data(), vertex_buffer_size);
    vmaUnmapMemory(m_allocator, staging_buffer.allocation);

    m_skybox_vertex_buffer = create_buffer(vertex_buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY);

    immediate_command([=](vk::CommandBuffer cmd) {
        vk::BufferCopy copy{ .srcOffset = 0, .dstOffset = 0, .size = vertex_buffer_size };
        cmd.copyBuffer(staging_buffer.buffer, m_skybox_vertex_buffer.buffer, copy);
    });

    m_deletion_queue.enqueue([=, copy = m_skybox_vertex_buffer]() {
        vmaDestroyBuffer(m_allocator, copy.buffer,
            copy.allocation);
    });

    vmaDestroyBuffer(m_allocator, staging_buffer.buffer, staging_buffer.allocation);

    const size_t index_buffer_size = skybox_indices.size() * sizeof(uint32_t);
    staging_buffer = create_buffer(index_buffer_size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    vmaMapMemory(m_allocator, staging_buffer.allocation, &data);
    memcpy(data, skybox_indices.data(), index_buffer_size);
    vmaUnmapMemory(m_allocator, staging_buffer.allocation);

    m_skybox_index_buffer = create_buffer(index_buffer_size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY);

    immediate_command([=](vk::CommandBuffer cmd) {
        vk::BufferCopy copy{ .srcOffset = 0, .dstOffset = 0, .size = index_buffer_size };
        cmd.copyBuffer(staging_buffer.buffer, m_skybox_index_buffer.buffer, copy);
    });

    m_deletion_queue.enqueue([=, copy = m_skybox_index_buffer]() {
        vmaDestroyBuffer(m_allocator, copy.buffer,
            copy.allocation);
    });

    vmaDestroyBuffer(m_allocator, staging_buffer.buffer, staging_buffer.allocation);
}

void Renderer::create_instance() {
    if (validation_enabled && !check_validation_layer_support(validation_layers))
        throw std::runtime_error("Validation layers unavailable");

    vk::ApplicationInfo app_info{
        .pApplicationName   = "Boa Application",
        .applicationVersion = 1,
        .pEngineName        = "Boa",
        .engineVersion      = 1,
        .apiVersion         = VK_API_VERSION_1_2
    };

    vk::InstanceCreateInfo create_info{};
    create_info.pApplicationInfo = &app_info;

    auto required_extensions = get_required_extensions(validation_enabled);
    std::vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    for (size_t i = 0; i < required_extensions.size(); i++) {
        if (!std::any_of(extensions.begin(), extensions.end(),
                    [&ext=required_extensions[i]] (const auto &e) {
                        return strcmp(e.extensionName, ext) == 0;
                    })) {
            throw std::runtime_error("Missing required extension");
        }
    }

    create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    create_info.ppEnabledExtensionNames = required_extensions.data();
    create_info.enabledLayerCount = 0;

    vk::DebugUtilsMessengerCreateInfoEXT debug_create_info;
    if (validation_enabled) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
        populate_debug_messenger_create_info(debug_create_info, &debug_callback);
        create_info.pNext = (vk::DebugUtilsMessengerCreateInfoEXT *)(&debug_create_info);
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    try {
        m_instance = vk::createInstanceUnique(create_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create instance");
    }
}

void Renderer::setup_debug_messenger() {
    if (!validation_enabled)
        return;

    vk::DebugUtilsMessengerCreateInfoEXT create_info;
    populate_debug_messenger_create_info(create_info, debug_callback);

    VkResult res = create_debug_utils_messenger_ext(
        m_instance.get(),
        reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT *>(&create_info),
        nullptr,
        &m_debug_messenger
    );

    if (res != VK_SUCCESS)
        throw std::runtime_error("Failed to setup debug messenger");
}

VKAPI_ATTR VkBool32 VKAPI_CALL Renderer::debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *p_callback_data,
    void *p_user_data
) {
    switch (message_severity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        LOG_INFO("(Validation) {}", p_callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        LOG_WARN("(Validation) {}", p_callback_data->pMessage);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        LOG_FAIL("(Validation) {}", p_callback_data->pMessage);
        break;
    default:
        break;
    }

    return VK_FALSE;
}

vk::SampleCountFlagBits Renderer::get_max_sample_count() const {
    const vk::SampleCountFlagBits descending_sample_counts[] = {
        vk::SampleCountFlagBits::e64,
        vk::SampleCountFlagBits::e32,
        vk::SampleCountFlagBits::e16,
        vk::SampleCountFlagBits::e8,
        vk::SampleCountFlagBits::e4,
        vk::SampleCountFlagBits::e2,
    };

    vk::SampleCountFlags counts = m_device_properties.limits.framebufferColorSampleCounts
        & m_device_properties.limits.framebufferDepthSampleCounts;

    for (auto sample_count : descending_sample_counts) {
        if (counts & sample_count)
            return sample_count;
    }

    return vk::SampleCountFlagBits::e1;
}

void Renderer::select_physical_device() {
    std::vector<vk::PhysicalDevice> devices =
        m_instance.get().enumeratePhysicalDevices();

    if (devices.empty())
        throw std::runtime_error("Failed to find vulkan-compatible GPU");

    for (const auto &device : devices) {
        if (check_device(device)) {
            m_physical_device = device;
            m_device_properties = m_physical_device.getProperties();
            m_msaa_samples = get_max_sample_count();
            break;
        }
    }

    if (!m_physical_device)
        throw std::runtime_error("Failed to find suitable GPU");

    LOG_INFO("(Renderer) Physical device '{}' selected", m_device_properties.deviceName);
    LOG_INFO("(Renderer) Max MSAA samples: {}", static_cast<int>(m_msaa_samples));
}

void Renderer::create_logical_device() {
    QueueFamilyIndices indices = find_queue_families(m_physical_device);

    std::vector<vk::DeviceQueueCreateInfo> q_create_infos;
    std::set<uint32_t> unique_q_families = { indices.graphics_family.value(), indices.present_family.value() };

    float q_priority = 1.0f;
    for (uint32_t q_family : unique_q_families) {
        q_create_infos.push_back(vk::DeviceQueueCreateInfo{
            .flags              = vk::DeviceQueueCreateFlags(),
            .queueFamilyIndex   = q_family,
            .queueCount         = 1,
            .pQueuePriorities   = &q_priority
        });
    }

    vk::PhysicalDeviceFeatures device_features{
        .samplerAnisotropy                      = true,
        .shaderSampledImageArrayDynamicIndexing = true,
    };

    vk::PhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{
        .descriptorBindingPartiallyBound        = true,
    };

    vk::PhysicalDeviceImagelessFramebufferFeatures imageless_framebuffer_features{
        .pNext                  = &descriptor_indexing_features,
        .imagelessFramebuffer   = true,
    };

    vk::DeviceCreateInfo create_info{
        .pNext                      = &imageless_framebuffer_features,
        .queueCreateInfoCount       = static_cast<uint32_t>(q_create_infos.size()),
        .pQueueCreateInfos          = q_create_infos.data(),
        .enabledExtensionCount      = static_cast<uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames    = device_extensions.data(),
        .pEnabledFeatures           = &device_features,
    };

    if (validation_enabled) {
        create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    try {
        m_device = m_physical_device.createDeviceUnique(create_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create logical device");
    }

    m_device.get().getQueue(indices.graphics_family.value(), 0, &m_graphics_queue);
    m_device.get().getQueue(indices.present_family.value(), 0, &m_present_queue);
}

void Renderer::create_surface() {
    vk::SurfaceKHR temp_surface;
    if (m_window.create_window_surface(static_cast<VkInstance>(m_instance.get()), reinterpret_cast<VkSurfaceKHR *>(&temp_surface)) != VK_SUCCESS)
        throw std::runtime_error("Failed to create window surface");
    m_surface = vk::UniqueSurfaceKHR(temp_surface, m_instance.get());
}

Renderer::SwapChainSupportDetails Renderer::query_swap_chain_support(vk::PhysicalDevice device) const {
    SwapChainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(m_surface.get());
    details.formats = device.getSurfaceFormatsKHR(m_surface.get());
    details.present_modes = device.getSurfacePresentModesKHR(m_surface.get());

    return details;
}

bool Renderer::check_device(vk::PhysicalDevice device) const {
    QueueFamilyIndices indices = find_queue_families(device);

    bool extensions_supported = check_device_extension_support(device);

    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support = query_swap_chain_support(device);
        swap_chain_adequate = !swap_chain_support.formats.empty() && !swap_chain_support.present_modes.empty();
    }

    vk::PhysicalDeviceFeatures supported_features = device.getFeatures();

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features> more_features;
    device.getFeatures2(&more_features.get<vk::PhysicalDeviceFeatures2>());

    return indices.is_complete() &&
        extensions_supported &&
        swap_chain_adequate &&
        supported_features.samplerAnisotropy &&
        more_features.get<vk::PhysicalDeviceVulkan12Features>().imagelessFramebuffer;
}

bool Renderer::check_device_extension_support(vk::PhysicalDevice device) const {
    std::vector<vk::ExtensionProperties> available_extensions =
        device.enumerateDeviceExtensionProperties();

    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto &extension : available_extensions)
        required_extensions.erase(extension.extensionName);

    return required_extensions.empty();
}

Renderer::QueueFamilyIndices Renderer::find_queue_families(vk::PhysicalDevice device) const {
    QueueFamilyIndices indices;

    auto queue_families = device.getQueueFamilyProperties();

    size_t i = 0;
    std::find_if(queue_families.begin(), queue_families.end(), [&](const auto &q_fam) {
        if (q_fam.queueFlags & vk::QueueFlagBits::eGraphics)
            indices.graphics_family = i;
        if (device.getSurfaceSupportKHR(i, m_surface.get()))
            indices.present_family = i;
        if (indices.is_complete())
            return true;

        i++;
        return false;
    });

    return indices;
}

vk::ImageView Renderer::create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect_flags, uint32_t mip_levels) const {
    vk::ImageViewCreateInfo view_info{
        .image              = image,
        .viewType           = vk::ImageViewType::e2D,
        .format             = format,
        .subresourceRange   = {
            .aspectMask     = aspect_flags,
            .baseMipLevel   = 0,
            .levelCount     = mip_levels,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };

    vk::ImageView image_view;
    try {
        image_view = m_device.get().createImageView(view_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create image view");
    }

    return image_view;
}

void Renderer::create_swapchain() {
    SwapChainSupportDetails swapchain_support = query_swap_chain_support(m_physical_device);

    vk::SurfaceFormatKHR surface_format = choose_swap_surface_format(swapchain_support.formats);
    vk::PresentModeKHR present_mode = choose_swap_present_mode(swapchain_support.present_modes);
    vk::Extent2D extent = choose_swap_extent(swapchain_support.capabilities, m_window);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0
            && image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    // NOTE: use VK_IMAGE_USAGE_TRANSFER_DST_BIT for post-processing images
    //       if we're not going to render directly to them
    //create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    vk::SwapchainCreateInfoKHR create_info{
        .surface            = m_surface.get(),
        .minImageCount      = image_count,
        .imageFormat        = surface_format.format,
        .imageColorSpace    = surface_format.colorSpace,
        .imageExtent        = extent,
        .imageArrayLayers   = 1,
        .imageUsage         = vk::ImageUsageFlagBits::eColorAttachment
    };

    QueueFamilyIndices indices = find_queue_families(m_physical_device);
    uint32_t queue_family_indices[] = { indices.graphics_family.value(), indices.present_family.value() };

    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = vk::SharingMode::eConcurrent;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        create_info.imageSharingMode = vk::SharingMode::eExclusive;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swapchain_support.capabilities.currentTransform;
    create_info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = vk::SwapchainKHR(nullptr);

    try {
        m_swapchain = m_device.get().createSwapchainKHR(create_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create swap chain");
    }

    m_device_format_properties = m_physical_device.getFormatProperties(surface_format.format);

    m_swapchain_format = surface_format.format;
    m_swapchain_images = m_device.get().getSwapchainImagesKHR(m_swapchain);
    m_swapchain_image_views.resize(m_swapchain_images.size(), VK_NULL_HANDLE);
    for (size_t i = 0; i < m_swapchain_images.size(); i++)
        m_swapchain_image_views[i] = create_image_view(m_swapchain_images[i], m_swapchain_format, vk::ImageAspectFlagBits::eColor);

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroySwapchainKHR(m_swapchain);
        for (size_t i = 0; i < m_swapchain_image_views.size(); i++)
            m_device.get().destroyImageView(m_swapchain_image_views[i]);
    });

    vk::Extent3D img_extent = {
        m_window_extent.width,
        m_window_extent.height,
        1,
    };

    // TODO check for supported formats like last time
    m_depth_format = vk::Format::eD32Sfloat;

    vk::ImageCreateInfo depth_img_create_info = image_create_info(m_depth_format,
        vk::ImageUsageFlagBits::eDepthStencilAttachment, img_extent, 1, m_msaa_samples);

    VmaAllocationCreateInfo img_alloc_info{
        .usage          = VMA_MEMORY_USAGE_GPU_ONLY,
        .requiredFlags  = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    vmaCreateImage(
        m_allocator,
        reinterpret_cast<VkImageCreateInfo *>(&depth_img_create_info),
        &img_alloc_info,
        reinterpret_cast<VkImage *>(&m_depth_image.image),
        &m_depth_image.allocation,
        nullptr);

    vk::ImageViewCreateInfo depth_img_view_create_info
        = image_view_create_info(m_depth_format, m_depth_image.image, vk::ImageAspectFlagBits::eDepth);

    try {
        m_depth_image_view = m_device.get().createImageView(depth_img_view_create_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create depth image view");
    }

    vk::ImageCreateInfo msaa_img_create_info = image_create_info(m_swapchain_format,
        vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
        img_extent, 1, m_msaa_samples);

    vmaCreateImage(
        m_allocator,
        reinterpret_cast<VkImageCreateInfo *>(&msaa_img_create_info),
        &img_alloc_info,
        reinterpret_cast<VkImage *>(&m_msaa_image.image),
        &m_msaa_image.allocation,
        nullptr);

    vk::ImageViewCreateInfo msaa_img_view_create_info
        = image_view_create_info(m_swapchain_format, m_msaa_image.image, vk::ImageAspectFlagBits::eColor);

    try {
        m_msaa_image_view = m_device.get().createImageView(msaa_img_view_create_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create MSAA image view");
    }

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroyImageView(m_depth_image_view);
        m_device.get().destroyImageView(m_msaa_image_view);
        vmaDestroyImage(m_allocator, m_depth_image.image, m_depth_image.allocation);
        vmaDestroyImage(m_allocator, m_msaa_image.image, m_msaa_image.allocation);
    });
}

void Renderer::create_commands() {
    uint32_t graphics_family_index = find_queue_families(m_physical_device).graphics_family.value();

    try {
        m_upload_context.command_pool = m_device.get().createCommandPool(command_pool_create_info(graphics_family_index));
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create command pool");
    }

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroyCommandPool(m_upload_context.command_pool);
    });

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        try {
            m_frames[i].command_pool = m_device.get().createCommandPool(command_pool_create_info(graphics_family_index, vk::CommandPoolCreateFlagBits::eResetCommandBuffer));
            m_frames[i].command_buffer = m_device.get().allocateCommandBuffers(command_buffer_allocate_info(m_frames[i].command_pool))[0];
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create command pool or buffers");
        }

        m_deletion_queue.enqueue([=]() {
            m_device.get().destroyCommandPool(m_frames[i].command_pool);
        });
    }
}

void Renderer::create_default_renderpass() {
    vk::AttachmentDescription color_attachment{
        .format         = m_swapchain_format,
        .samples        = m_msaa_samples,
        .loadOp         = vk::AttachmentLoadOp::eClear,
        //.storeOp        = vk::AttachmentStoreOp::eDontCare,
        .storeOp        = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout  = vk::ImageLayout::eUndefined,
        .finalLayout    = vk::ImageLayout::eColorAttachmentOptimal,
    };

    vk::AttachmentDescription depth_attachment{
        //.format         = find_depth_format(m_physical_device),
        .format         = m_depth_format,
        .samples        = m_msaa_samples,
        .loadOp         = vk::AttachmentLoadOp::eClear,
        // don't care, because depth data won't be used after drawing has finished
        .storeOp        = vk::AttachmentStoreOp::eDontCare,
        .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout  = vk::ImageLayout::eUndefined,
        .finalLayout    = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    vk::AttachmentDescription color_attachment_resolve{
        .format         = m_swapchain_format,
        .samples        = vk::SampleCountFlagBits::e1,
        .loadOp         = vk::AttachmentLoadOp::eDontCare,
        .storeOp        = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp  = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout  = vk::ImageLayout::eUndefined,
        .finalLayout    = vk::ImageLayout::ePresentSrcKHR,
    };

    vk::AttachmentReference color_attachment_ref{
        .attachment = 0,
        .layout     = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentReference depth_attachment_ref{
        .attachment = 1,
        .layout     = vk::ImageLayout::eDepthStencilAttachmentOptimal,
    };

    vk::AttachmentReference color_attachment_resolve_ref{
        .attachment = 2,
        .layout     = vk::ImageLayout::eColorAttachmentOptimal,
    };

    vk::SubpassDescription subpass{
        .pipelineBindPoint          = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount       = 1,
        .pColorAttachments          = &color_attachment_ref,
        .pResolveAttachments        = &color_attachment_resolve_ref,
        .pDepthStencilAttachment    = &depth_attachment_ref,
    };

    vk::SubpassDependency dependency{
        .srcSubpass     = VK_SUBPASS_EXTERNAL,
        .dstSubpass     = 0,
        .srcStageMask   = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .dstStageMask   = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask  = vk::AccessFlagBits::eNoneKHR,
        .dstAccessMask  = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
    };

    const std::array<vk::AttachmentDescription, 3> attachments = {
        color_attachment,
        depth_attachment,
        color_attachment_resolve
    };

    vk::RenderPassCreateInfo render_pass_info{
        .flags              = vk::RenderPassCreateFlags(),
        .attachmentCount    = static_cast<uint32_t>(attachments.size()),
        .pAttachments       = attachments.data(),
        .subpassCount       = 1,
        .pSubpasses         = &subpass,
        .dependencyCount    = 1,
        .pDependencies      = &dependency,
    };

    try {
        m_renderpass = m_device.get().createRenderPass(render_pass_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create render pass");
    }

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroyRenderPass(m_renderpass);
    });
}

void Renderer::create_framebuffer() {
    vk::FramebufferAttachmentImageInfo depth_attach{
        .usage              = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .width              = m_window_extent.width,
        .height             = m_window_extent.height,
        .layerCount         = 1,
        .viewFormatCount    = 1,
        .pViewFormats       = &m_depth_format,
    };

    vk::FramebufferAttachmentImageInfo color_attach{
        .usage              = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
        .width              = m_window_extent.width,
        .height             = m_window_extent.height,
        .layerCount         = 1,
        .viewFormatCount    = 1,
        .pViewFormats       = &m_swapchain_format,
    };

    vk::FramebufferAttachmentImageInfo color_attach_resolve{
        .usage              = vk::ImageUsageFlagBits::eColorAttachment,
        .width              = m_window_extent.width,
        .height             = m_window_extent.height,
        .layerCount         = 1,
        .viewFormatCount    = 1,
        .pViewFormats       = &m_swapchain_format,
    };

    vk::FramebufferAttachmentImageInfo attach_infos[] = {
        color_attach,
        depth_attach,
        color_attach_resolve
    };

    vk::FramebufferAttachmentsCreateInfo attach_create_info{
        .attachmentImageInfoCount   = 3,
        .pAttachmentImageInfos      = attach_infos,
    };

    vk::FramebufferCreateInfo create_info{
        .pNext              = &attach_create_info,
        .flags              = vk::FramebufferCreateFlagBits::eImageless,
        .renderPass         = m_renderpass,
        .attachmentCount    = 3,
        .pAttachments       = nullptr,
        .width              = m_window_extent.width,
        .height             = m_window_extent.height,
        .layers             = 1,
    };

    try {
        m_framebuffer = m_device.get().createFramebuffer(create_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create framebuffer");
    }

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroyFramebuffer(m_framebuffer);
    });
}

void Renderer::create_sync_objects() {
    try {
        // create fence for upload context (used for immediate commands)
        m_upload_context.upload_fence = m_device.get().createFence(vk::FenceCreateInfo{});
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create upload context fence");
    }

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroyFence(m_upload_context.upload_fence);
    });

    vk::FenceCreateInfo f_create_info{ .flags = vk::FenceCreateFlagBits::eSignaled };
    vk::SemaphoreCreateInfo s_create_info{};

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        try {
            m_frames[i].render_fence = m_device.get().createFence(f_create_info);
            m_frames[i].present_sem = m_device.get().createSemaphore(s_create_info);
            m_frames[i].render_sem = m_device.get().createSemaphore(s_create_info);
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create synchronization objects");
        }

        m_deletion_queue.enqueue([=]() {
            m_device.get().destroyFence(m_frames[i].render_fence);
            m_device.get().destroySemaphore(m_frames[i].present_sem);
            m_device.get().destroySemaphore(m_frames[i].render_sem);
        });
    }
}

vk::ShaderModule Renderer::load_shader(const char *path) {
    LOG_INFO("(Renderer) Loading shader module at '{}'", path);

    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open shader file");

    size_t file_size = (size_t)file.tellg();
    std::vector<char> file_buffer(file_size);

    file.seekg(0);
    file.read(file_buffer.data(), file_size);
    file.close();

    vk::ShaderModuleCreateInfo create_info{
        .codeSize   = file_buffer.size(),
        .pCode      = reinterpret_cast<uint32_t *>(file_buffer.data()),
    };

    vk::ShaderModule shader_module;
    try {
        shader_module = m_device.get().createShaderModule(create_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create shader module");
    }

    return shader_module;
}

Renderer::PerFrame &Renderer::current_frame() {
    return m_frames[m_frame % FRAMES_IN_FLIGHT];
}

VmaBuffer Renderer::create_buffer(size_t size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage) const {
    vk::BufferCreateInfo buffer_info{
        .size   = size,
        .usage  = usage,
    };

    VmaAllocationCreateInfo vma_alloc_info { .usage = memory_usage };

    VmaBuffer buffer;
    vmaCreateBuffer(m_allocator, (VkBufferCreateInfo *)&buffer_info, &vma_alloc_info, (VkBuffer *)&buffer.buffer, &buffer.allocation, nullptr);

    return buffer;
}

void Renderer::create_descriptors() {
    std::vector<vk::DescriptorPoolSize> sizes = {
        { vk::DescriptorType::eUniformBuffer,           1000 },
        { vk::DescriptorType::eSampler,                 1000 },
        { vk::DescriptorType::eCombinedImageSampler,    1000 },
    };

    vk::DescriptorPoolCreateInfo pool_info{
        .maxSets        = 10,
        .poolSizeCount  = (uint32_t)sizes.size(),
        .pPoolSizes     = sizes.data(),
    };

    try {
        m_descriptor_pool = m_device.get().createDescriptorPool(pool_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create descriptor pool");
    }

    vk::DescriptorSetLayoutBinding transform_binding{
        .binding            = 0,
        .descriptorType     = vk::DescriptorType::eUniformBuffer,
        .descriptorCount    = 1,
        .stageFlags         = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr,
    };

    vk::DescriptorSetLayoutCreateInfo set_info{
        .bindingCount   = 1,
        .pBindings      = &transform_binding,
    };

    try {
        m_descriptor_set_layout = m_device.get().createDescriptorSetLayout(set_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    vk::DescriptorSetLayoutBinding blinn_phong_binding{
        .binding            = 1,
        .descriptorType     = vk::DescriptorType::eUniformBuffer,
        .descriptorCount    = 1,
        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };

    vk::DescriptorSetLayoutBinding blinn_phong_bindings[] = { transform_binding, blinn_phong_binding };
    vk::DescriptorSetLayoutCreateInfo blinn_phong_set_info{
        .bindingCount   = 2,
        .pBindings      = blinn_phong_bindings,
    };

    try {
        m_blinn_phong_set_layout = m_device.get().createDescriptorSetLayout(blinn_phong_set_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create descriptor set layout");
    }

    vk::DescriptorSetLayoutBinding texture_bind{
        .binding            = 0,
        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount    = MAX_IMAGE_DESCRIPTORS,
        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };

    vk::DescriptorBindingFlags texture_binding_flags = vk::DescriptorBindingFlagBits::ePartiallyBound;
    vk::DescriptorSetLayoutBindingFlagsCreateInfo texture_bind_flags{
        .bindingCount   = 1,
        .pBindingFlags  = &texture_binding_flags,
    };

    vk::DescriptorSetLayoutCreateInfo texture_set_info{
        .pNext              = &texture_bind_flags,
        .bindingCount       = 1,
        .pBindings          = &texture_bind,
    };

    try {
        m_textures_set_layout = m_device.get().createDescriptorSetLayout(texture_set_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create descriptor set layout for textures");
    }

    vk::DescriptorSetLayoutBinding skybox_bind{
        .binding            = 0,
        .descriptorType     = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount    = 1,
        .stageFlags         = vk::ShaderStageFlagBits::eFragment,
        .pImmutableSamplers = nullptr,
    };

    vk::DescriptorSetLayoutCreateInfo skybox_set_info{
        .bindingCount       = 1,
        .pBindings          = &skybox_bind,
    };

    try {
        m_skybox_set_layout = m_device.get().createDescriptorSetLayout(skybox_set_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create descriptor set layout for textures");
    }

    for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
        m_frames[i].transformations_buffer =
            create_buffer(sizeof(Transformations), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);
        m_frames[i].blinn_phong_buffer =
            create_buffer(sizeof(BlinnPhong), vk::BufferUsageFlagBits::eUniformBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU);

        vk::DescriptorSetAllocateInfo alloc_info{
            .descriptorPool     = m_descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &m_descriptor_set_layout,
        };

        vk::DescriptorSetAllocateInfo blinn_phong_alloc_info{
            .descriptorPool     = m_descriptor_pool,
            .descriptorSetCount = 1,
            .pSetLayouts        = &m_blinn_phong_set_layout,
        };

        try {
            m_frames[i].parent_set = m_device.get().allocateDescriptorSets(alloc_info)[0];
            m_frames[i].parent_blinn_phong_set = m_device.get().allocateDescriptorSets(blinn_phong_alloc_info)[0];
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to allocate descriptor set");
        }

        vk::DescriptorBufferInfo buffer_info{
            .buffer = m_frames[i].transformations_buffer.buffer,
            .offset = 0,
            .range  = sizeof(Transformations),
        };

        vk::DescriptorBufferInfo blinn_phong_buffer_info{
            .buffer = m_frames[i].blinn_phong_buffer.buffer,
            .offset = 0,
            .range  = sizeof(BlinnPhong),
        };

        std::array<vk::WriteDescriptorSet, 3> set_writes{
            vk::WriteDescriptorSet{
                .dstSet             = m_frames[i].parent_set,
                .dstBinding         = 0,
                .dstArrayElement    = 0,
                .descriptorCount    = 1,
                .descriptorType     = vk::DescriptorType::eUniformBuffer,
                .pImageInfo         = nullptr,
                .pBufferInfo        = &buffer_info,
                .pTexelBufferView   = nullptr,
            },
            vk::WriteDescriptorSet{
                .dstSet             = m_frames[i].parent_blinn_phong_set,
                .dstBinding         = 0,
                .dstArrayElement    = 0,
                .descriptorCount    = 1,
                .descriptorType     = vk::DescriptorType::eUniformBuffer,
                .pImageInfo         = nullptr,
                .pBufferInfo        = &buffer_info,
                .pTexelBufferView   = nullptr,
            },
            vk::WriteDescriptorSet{
                .dstSet             = m_frames[i].parent_blinn_phong_set,
                .dstBinding         = 1,
                .dstArrayElement    = 0,
                .descriptorCount    = 1,
                .descriptorType     = vk::DescriptorType::eUniformBuffer,
                .pImageInfo         = nullptr,
                .pBufferInfo        = &blinn_phong_buffer_info,
                .pTexelBufferView   = nullptr,
            },
        };

        m_device.get().updateDescriptorSets(set_writes, 0);
    }

    m_deletion_queue.enqueue([&]() {
        m_device.get().destroyDescriptorSetLayout(m_descriptor_set_layout);
        m_device.get().destroyDescriptorSetLayout(m_textures_set_layout);
        m_device.get().destroyDescriptorSetLayout(m_blinn_phong_set_layout);
        m_device.get().destroyDescriptorSetLayout(m_skybox_set_layout);
        m_device.get().destroyDescriptorPool(m_descriptor_pool);

        for (size_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
            vmaDestroyBuffer(m_allocator, m_frames[i].transformations_buffer.buffer, m_frames[i].transformations_buffer.allocation);
            vmaDestroyBuffer(m_allocator, m_frames[i].blinn_phong_buffer.buffer, m_frames[i].blinn_phong_buffer.allocation);
        }
    });
}

void Renderer::create_pipelines() {
    LOG_INFO("(Renderer) Creating pipelines");

    vk::ShaderModule untextured_frag = load_shader("shaders/untextured/untextured_frag.spv");
    vk::ShaderModule untextured_vert = load_shader("shaders/untextured/untextured_vert.spv");
    vk::ShaderModule textured_frag = load_shader("shaders/textured/textured_frag.spv");
    vk::ShaderModule textured_vert = load_shader("shaders/textured/textured_vert.spv");
    vk::ShaderModule bounding_box_frag = load_shader("shaders/bounding_box/bounding_box_frag.spv");
    vk::ShaderModule bounding_box_vert = load_shader("shaders/bounding_box/bounding_box_vert.spv");
    vk::ShaderModule textured_blinn_phong_frag = load_shader("shaders/blinn_phong/textured_blinn_phong_frag.spv");
    vk::ShaderModule textured_blinn_phong_vert = load_shader("shaders/blinn_phong/textured_blinn_phong_vert.spv");
    vk::ShaderModule untextured_blinn_phong_frag = load_shader("shaders/blinn_phong/untextured_blinn_phong_frag.spv");
    vk::ShaderModule untextured_blinn_phong_vert = load_shader("shaders/blinn_phong/untextured_blinn_phong_vert.spv");
    vk::ShaderModule skybox_frag = load_shader("shaders/skybox/skybox_frag.spv");
    vk::ShaderModule skybox_vert = load_shader("shaders/skybox/skybox_vert.spv");

    PipelineContext pipeline_ctx;

    vk::PipelineLayoutCreateInfo untextured_layout_info = pipeline_layout_create_info();
    vk::Pipeline untextured_pipeline,
        textured_pipeline,
        bounding_box_pipeline,
        untextured_blinn_phong_pipeline,
        textured_blinn_phong_pipeline,
        skybox_pipeline;
    vk::PipelineLayout untextured_pipeline_layout,
        textured_pipeline_layout,
        bounding_box_pipeline_layout,
        untextured_blinn_phong_pipeline_layout,
        textured_blinn_phong_pipeline_layout,
        skybox_pipeline_layout;

    vk::PushConstantRange push_constants{
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset     = 0,
        .size       = sizeof(PushConstants),
    };

    auto attrib_desc = Vertex::get_attribute_descriptions();
    auto binding_desc = Vertex::get_binding_description();

    // UNTEXTURED PIPELINE
    {
        untextured_layout_info.setLayoutCount = 1;
        untextured_layout_info.pSetLayouts = &m_descriptor_set_layout;
        untextured_layout_info.pushConstantRangeCount = 1;
        untextured_layout_info.pPushConstantRanges = &push_constants;

        try {
            untextured_pipeline_layout = m_device.get().createPipelineLayout(untextured_layout_info);
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create untextured pipeline layout");
        }

        pipeline_ctx.input_assembly = input_assembly_create_info(vk::PrimitiveTopology::eTriangleList);

        pipeline_ctx.vertex_input_info.pVertexAttributeDescriptions = attrib_desc.data();
        pipeline_ctx.vertex_input_info.vertexAttributeDescriptionCount = attrib_desc.size();
        pipeline_ctx.vertex_input_info.pVertexBindingDescriptions = &binding_desc;
        pipeline_ctx.vertex_input_info.vertexBindingDescriptionCount = 1;

        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, untextured_vert));
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, untextured_frag));

        pipeline_ctx.rasterizer = rasterization_state_create_info(vk::PolygonMode::eFill);
        pipeline_ctx.multisample = multisample_state_create_info(m_msaa_samples);
        pipeline_ctx.color_blend_attachment = color_blend_attachment_state();
        pipeline_ctx.depth_stencil = depth_stencil_create_info(true, true, vk::CompareOp::eLess);
        pipeline_ctx.pipeline_layout = untextured_pipeline_layout;

        untextured_pipeline = pipeline_ctx.build(m_device.get(), m_renderpass);
        m_asset_manager.create_material(untextured_pipeline, untextured_pipeline_layout);
    }

    // TEXTURED PIPELINE
    {
        vk::PipelineLayoutCreateInfo textured_layout_info = untextured_layout_info;

        vk::DescriptorSetLayout textured_set_layouts[] = { m_descriptor_set_layout, m_textures_set_layout };

        textured_layout_info.setLayoutCount = 2;
        textured_layout_info.pSetLayouts = textured_set_layouts;

        try {
            textured_pipeline_layout = m_device.get().createPipelineLayout(textured_layout_info);
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create textured pipeline layout");
        }

        pipeline_ctx.shader_stages.clear();
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, textured_vert));
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, textured_frag));

        pipeline_ctx.pipeline_layout = textured_pipeline_layout;

        textured_pipeline = pipeline_ctx.build(m_device.get(), m_renderpass);
        m_asset_manager.create_material(textured_pipeline, textured_pipeline_layout);
    }

    // BOUNDING BOX LINES PIPELINE
    {
        vk::PipelineLayoutCreateInfo bounding_box_layout_info = untextured_layout_info;

        vk::DescriptorSetLayout bounding_box_set_layouts[] = { m_descriptor_set_layout };

        bounding_box_layout_info.setLayoutCount = 1;
        bounding_box_layout_info.pSetLayouts = bounding_box_set_layouts;

        try {
            bounding_box_pipeline_layout = m_device.get().createPipelineLayout(bounding_box_layout_info);
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create bounding box pipeline layout");
        }

        pipeline_ctx.shader_stages.clear();
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, bounding_box_vert));
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, bounding_box_frag));
        pipeline_ctx.input_assembly = input_assembly_create_info(vk::PrimitiveTopology::eLineList);
        pipeline_ctx.depth_stencil = depth_stencil_create_info(true, true, vk::CompareOp::eLess);

        pipeline_ctx.pipeline_layout = bounding_box_pipeline_layout;

        bounding_box_pipeline = pipeline_ctx.build(m_device.get(), m_renderpass);
        m_asset_manager.create_material(bounding_box_pipeline, bounding_box_pipeline_layout);
    }

    // UNTEXTURED BLINN-PHONG PIPELINE
    {
        vk::PipelineLayoutCreateInfo blinn_phong_layout_info = untextured_layout_info;

        vk::DescriptorSetLayout blinn_phong_set_layouts[] = { m_blinn_phong_set_layout };

        blinn_phong_layout_info.setLayoutCount = 1;
        blinn_phong_layout_info.pSetLayouts = blinn_phong_set_layouts;

        try {
            untextured_blinn_phong_pipeline_layout = m_device.get().createPipelineLayout(blinn_phong_layout_info);
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create untextured Blinn-Phong pipeline layout");
        }

        pipeline_ctx.shader_stages.clear();
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, untextured_blinn_phong_vert));
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, untextured_blinn_phong_frag));
        pipeline_ctx.input_assembly = input_assembly_create_info(vk::PrimitiveTopology::eTriangleList);
        pipeline_ctx.depth_stencil = depth_stencil_create_info(true, true, vk::CompareOp::eLess);

        pipeline_ctx.pipeline_layout = untextured_blinn_phong_pipeline_layout;

        untextured_blinn_phong_pipeline = pipeline_ctx.build(m_device.get(), m_renderpass);
        m_asset_manager.create_material(untextured_blinn_phong_pipeline, untextured_blinn_phong_pipeline_layout);
    }

    // TEXTURED BLINN-PHONG PIPELINE
    {
        vk::PipelineLayoutCreateInfo blinn_phong_layout_info = untextured_layout_info;

        vk::DescriptorSetLayout blinn_phong_set_layouts[] = { m_blinn_phong_set_layout, m_textures_set_layout };

        blinn_phong_layout_info.setLayoutCount = 2;
        blinn_phong_layout_info.pSetLayouts = blinn_phong_set_layouts;

        try {
            textured_blinn_phong_pipeline_layout = m_device.get().createPipelineLayout(blinn_phong_layout_info);
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create textured Blinn-Phong pipeline layout");
        }

        pipeline_ctx.shader_stages.clear();
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, textured_blinn_phong_vert));
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, textured_blinn_phong_frag));

        pipeline_ctx.pipeline_layout = textured_blinn_phong_pipeline_layout;

        textured_blinn_phong_pipeline = pipeline_ctx.build(m_device.get(), m_renderpass);
        m_asset_manager.create_material(textured_blinn_phong_pipeline, textured_blinn_phong_pipeline_layout);
    }

    // SKYBOX PIPELINE
    {
        vk::PipelineLayoutCreateInfo skybox_layout_info = pipeline_layout_create_info();

        vk::DescriptorSetLayout skybox_set_layouts[] = { m_descriptor_set_layout, m_skybox_set_layout };

        skybox_layout_info.setLayoutCount = 2;
        skybox_layout_info.pSetLayouts = skybox_set_layouts;

        try {
            skybox_pipeline_layout = m_device.get().createPipelineLayout(skybox_layout_info);
        } catch (const vk::SystemError &err) {
            throw std::runtime_error("Failed to create untextured pipeline layout");
        }

        pipeline_ctx.shader_stages.clear();
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, skybox_vert));
        pipeline_ctx.shader_stages.push_back(
            pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, skybox_frag));

        pipeline_ctx.depth_stencil = depth_stencil_create_info(true, true, vk::CompareOp::eLessOrEqual);
        pipeline_ctx.rasterizer.cullMode = vk::CullModeFlagBits::eNone;
        //pipeline_ctx.rasterizer.frontFace = vk::FrontFace::eClockwise;
        //pipeline_ctx.depth_stencil.stencilTestEnable = false;
        pipeline_ctx.pipeline_layout = skybox_pipeline_layout;

        skybox_pipeline = pipeline_ctx.build(m_device.get(), m_renderpass);

        m_skybox_pipeline = skybox_pipeline;
        m_skybox_pipeline_layout = skybox_pipeline_layout;
    }

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroyPipeline(untextured_pipeline);
        m_device.get().destroyPipeline(textured_pipeline);
        m_device.get().destroyPipeline(bounding_box_pipeline);
        m_device.get().destroyPipeline(untextured_blinn_phong_pipeline);
        m_device.get().destroyPipeline(textured_blinn_phong_pipeline);
        m_device.get().destroyPipeline(skybox_pipeline);
        m_device.get().destroyPipelineLayout(untextured_pipeline_layout);
        m_device.get().destroyPipelineLayout(textured_pipeline_layout);
        m_device.get().destroyPipelineLayout(bounding_box_pipeline_layout);
        m_device.get().destroyPipelineLayout(untextured_blinn_phong_pipeline_layout);
        m_device.get().destroyPipelineLayout(textured_blinn_phong_pipeline_layout);
        m_device.get().destroyPipelineLayout(skybox_pipeline_layout);
    });

    m_device.get().destroyShaderModule(untextured_frag);
    m_device.get().destroyShaderModule(untextured_vert);
    m_device.get().destroyShaderModule(textured_frag);
    m_device.get().destroyShaderModule(textured_vert);
    m_device.get().destroyShaderModule(bounding_box_frag);
    m_device.get().destroyShaderModule(bounding_box_vert);
    m_device.get().destroyShaderModule(untextured_blinn_phong_frag);
    m_device.get().destroyShaderModule(untextured_blinn_phong_vert);
    m_device.get().destroyShaderModule(textured_blinn_phong_frag);
    m_device.get().destroyShaderModule(textured_blinn_phong_vert);
    m_device.get().destroyShaderModule(skybox_frag);
    m_device.get().destroyShaderModule(skybox_vert);
}

void Renderer::immediate_command(std::function<void(vk::CommandBuffer cmd)> &&function) {
    vk::CommandBufferAllocateInfo cmd_alloc_info = command_buffer_allocate_info(m_upload_context.command_pool, 1);

    vk::CommandBuffer cmd = m_device.get().allocateCommandBuffers(cmd_alloc_info)[0];

    vk::CommandBufferBeginInfo cmd_begin_info = { .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit };
    cmd.begin(cmd_begin_info);

    function(cmd);

    try {
        cmd.end();
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to finalize immediate command buffer");
    }

    vk::SubmitInfo submit_info = { .commandBufferCount = 1, .pCommandBuffers = &cmd };
    try {
        m_graphics_queue.submit(submit_info, m_upload_context.upload_fence);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to submit immediate command buffer");
    }

    m_device.get().waitForFences(m_upload_context.upload_fence, 1, 1e9);
    m_device.get().resetFences(m_upload_context.upload_fence);

    // reset pool because eventually we'll submit multiple command buffers at a time
    m_device.get().resetCommandPool(m_upload_context.command_pool);
}

void Renderer::init_imgui() {
    IMGUI_CHECKVERSION();

    vk::DescriptorPoolSize pool_sizes[] = {
        { vk::DescriptorType::eSampler,                 1000 },
        { vk::DescriptorType::eCombinedImageSampler,    1000 },
        { vk::DescriptorType::eSampledImage,            1000 },
        { vk::DescriptorType::eStorageImage,            1000 },
        { vk::DescriptorType::eUniformTexelBuffer,      1000 },
        { vk::DescriptorType::eStorageTexelBuffer,      1000 },
        { vk::DescriptorType::eUniformBuffer,           1000 },
        { vk::DescriptorType::eStorageBuffer,           1000 },
        { vk::DescriptorType::eUniformBufferDynamic,    1000 },
        { vk::DescriptorType::eStorageBufferDynamic,    1000 },
        { vk::DescriptorType::eInputAttachment,         1000 },
    };

    vk::DescriptorPoolCreateInfo pool_info{
        .maxSets        = 1000,
        .poolSizeCount  = std::size(pool_sizes),
        .pPoolSizes     = pool_sizes,
    };

    vk::DescriptorPool imgui_pool;
    try {
        imgui_pool = m_device.get().createDescriptorPool(pool_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create descriptor pool for imgui");
    }

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(m_window.get_glfw_window(), true);
    ImGui_ImplVulkan_InitInfo init_info{
        .Instance       = m_instance.get(),
        .PhysicalDevice = m_physical_device,
        .Device         = m_device.get(),
        .Queue          = m_graphics_queue,
        .DescriptorPool = imgui_pool,
        .MinImageCount  = 3,
        .ImageCount     = 3,
        .MSAASamples    = static_cast<VkSampleCountFlagBits>(m_msaa_samples),
    };

    ImGui_ImplVulkan_Init(&init_info, m_renderpass);
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowRounding = 5.0f;

    immediate_command([&](vk::CommandBuffer cmd) {
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();

    m_deletion_queue.enqueue([=]() {
        m_device.get().destroyDescriptorPool(imgui_pool);
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    });
}

}
