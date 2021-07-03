#include <GLFW/glfw3.h>
#include "boa/util.h"

bool boa::check_validation_layer_support(const std::vector<const char *> &layers) {
    auto available_layers = vk::enumerateInstanceLayerProperties();

    for (const char *layer_name : layers) {
        if (!std::any_of(available_layers.begin(), available_layers.end(),
                    [&layer_name](const auto &avail_layer) {
                        return strcmp(avail_layer.layerName, layer_name) == 0;
                    })) {
            return false;
        }
    }

    return true;
}

std::vector<const char *> boa::get_required_extensions(bool validation_enabled) {
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (validation_enabled)
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
}

void boa::populate_debug_messenger_create_info(vk::DebugUtilsMessengerCreateInfoEXT &create_info, PFN_vkDebugUtilsMessengerCallbackEXT debug_callback) {
    create_info = vk::DebugUtilsMessengerCreateInfoEXT{
        .flags              = {},
        .messageSeverity    = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType        = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback    = debug_callback
    };
}

VkResult boa::create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
        const VkAllocationCallbacks *p_allocator, VkDebugUtilsMessengerEXT *p_debug_messenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void boa::destroy_debug_utils_messenger_ext(vk::Instance instance, vk::DebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks *p_allocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debug_messenger, p_allocator);
    }
}

vk::SurfaceFormatKHR boa::choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR> &available_formats) {
    for (const auto &available_format : available_formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb &&
                available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }

    return available_formats[0];
}

vk::PresentModeKHR boa::choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &available_present_modes) {
    for (const auto &mode : available_present_modes) {
        if (mode == vk::PresentModeKHR::eMailbox)
            return mode;
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D boa::choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, boa::Window &window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    } else {
        int w, h;
        window.get_framebuffer_size(w, h);

        vk::Extent2D actual_extent = {
            static_cast<uint32_t>(w),
            static_cast<uint32_t>(h),
        };

        actual_extent.width = std::max(capabilities.minImageExtent.width,
                std::min(capabilities.maxImageExtent.width, actual_extent.width));
        actual_extent.height = std::max(capabilities.minImageExtent.height,
                std::min(capabilities.maxImageExtent.height, actual_extent.height));

        return actual_extent;
    }
}

vk::Format boa::find_supported_format(
    vk::PhysicalDevice physical_device,
    const std::vector<vk::Format> &candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
) {
    for (vk::Format format : candidates) {
        vk::FormatProperties properties;
        properties = physical_device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported format");
}

vk::Format boa::find_depth_format(vk::PhysicalDevice physical_device) {
    return find_supported_format(
        physical_device,
        {
            vk::Format::eD32Sfloat,
            vk::Format::eD32SfloatS8Uint,
            vk::Format::eD24UnormS8Uint,
        },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::Pipeline boa::PipelineContext::build(vk::Device device, vk::RenderPass renderpass) {
    vk::PipelineViewportStateCreateInfo viewport_state{
        .viewportCount  = 1,
        .pViewports     = nullptr,
        .scissorCount   = 1,
        .pScissors      = nullptr,
    };

    vk::DynamicState dynamic_states[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
    vk::PipelineDynamicStateCreateInfo dynamic_info{
        .dynamicStateCount  = 2,
        .pDynamicStates     = dynamic_states
    };

    vk::PipelineColorBlendStateCreateInfo color_blend{
        .logicOpEnable      = false,
        .logicOp            = vk::LogicOp::eCopy,
        .attachmentCount    = 1,
        .pAttachments       = &color_blend_attachment,
        .blendConstants     = std::array<float, 4>{
            0.0f, 0.0f,
            0.0f, 0.0f,
        },
    };

    vk::GraphicsPipelineCreateInfo pipeline_info{
        .stageCount             = (uint32_t)shader_stages.size(),
        .pStages                = shader_stages.data(),
        .pVertexInputState      = &vertex_input_info,
        .pInputAssemblyState    = &input_assembly,
        .pViewportState         = &viewport_state,
        .pRasterizationState    = &rasterizer,
        .pMultisampleState      = &multisampling,
        .pDepthStencilState     = &depth_stencil,
        .pColorBlendState       = &color_blend,
        .pDynamicState          = &dynamic_info,
        .layout                 = pipeline_layout,
        .renderPass             = renderpass,
        .subpass                = 0,
        .basePipelineHandle     = VK_NULL_HANDLE,
    };

    vk::Pipeline pipeline;
    try {
        pipeline = device.createGraphicsPipeline(nullptr, pipeline_info).value;
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create graphics pipeline");
    }

    return pipeline;
}