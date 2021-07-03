#ifndef BOA_UTIL_H
#define BOA_UTIL_H

#include "boa/window.h"
#include <vulkan/vulkan.hpp>
#include <vector>

namespace boa {

struct PipelineContext {
    vk::Pipeline build(vk::Device device, vk::RenderPass renderpass);

    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    vk::Viewport viewport;
    vk::Rect2D scissor;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    vk::PipelineMultisampleStateCreateInfo multisampling;
    vk::PipelineDepthStencilStateCreateInfo depth_stencil;
    vk::PipelineLayout pipeline_layout;
};

bool check_validation_layer_support(const std::vector<const char *> &layers);
std::vector<const char *> get_required_extensions(bool validation_enabled);
void populate_debug_messenger_create_info(vk::DebugUtilsMessengerCreateInfoEXT &create_info, PFN_vkDebugUtilsMessengerCallbackEXT debug_callback);
VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *p_create_info,
        const VkAllocationCallbacks *p_allocator, VkDebugUtilsMessengerEXT *p_debug_messenger);
void destroy_debug_utils_messenger_ext(vk::Instance instance, vk::DebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks *p_allocator);
vk::SurfaceFormatKHR choose_swap_surface_format(const std::vector<vk::SurfaceFormatKHR> &available_formats);
vk::PresentModeKHR choose_swap_present_mode(const std::vector<vk::PresentModeKHR> &available_present_modes);
vk::Extent2D choose_swap_extent(const vk::SurfaceCapabilitiesKHR &capabilities, boa::Window &window);
vk::Format find_supported_format(
    vk::PhysicalDevice physical_device,
    const std::vector<vk::Format> &candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features
);
vk::Format find_depth_format(vk::PhysicalDevice physical_device);

}

#endif