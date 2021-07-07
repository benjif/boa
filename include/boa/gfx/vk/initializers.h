#ifndef BOA_GFX_VK_INITIALIZERS_H
#define BOA_GFX_VK_INITIALIZERS_H

#include <vulkan/vulkan.hpp>

namespace boa {

vk::CommandPoolCreateInfo command_pool_create_info(uint32_t queue_family_index, vk::CommandPoolCreateFlags flags = {});
vk::CommandBufferAllocateInfo command_buffer_allocate_info(vk::CommandPool pool, uint32_t count = 1, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

vk::PipelineShaderStageCreateInfo pipeline_shader_stage_create_info(vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module);
vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info();
vk::PipelineInputAssemblyStateCreateInfo input_assembly_create_info(vk::PrimitiveTopology topology);
vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info(vk::PolygonMode polygon_mode);
vk::PipelineMultisampleStateCreateInfo multisample_state_create_info(vk::SampleCountFlagBits samples);
vk::PipelineColorBlendAttachmentState color_blend_attachment_state();
vk::PipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool depth_test, bool depth_write, vk::CompareOp compare_op);
vk::PipelineLayoutCreateInfo pipeline_layout_create_info();

vk::ImageCreateInfo image_create_info(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent, uint32_t mip_levels = 1,
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
vk::ImageViewCreateInfo image_view_create_info(vk::Format format, vk::Image image, vk::ImageAspectFlags aspect, uint32_t mip_levels = 1);

vk::SamplerCreateInfo sampler_create_info(vk::Filter filters, vk::SamplerAddressMode address_mode = vk::SamplerAddressMode::eRepeat);
vk::WriteDescriptorSet write_descriptor_image(vk::DescriptorType type, vk::DescriptorSet dst, vk::DescriptorImageInfo *image_info, uint32_t binding);

}

#endif