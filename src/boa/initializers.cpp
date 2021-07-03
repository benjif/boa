#include "boa/initializers.h"

vk::CommandPoolCreateInfo boa::command_pool_create_info(uint32_t queue_family_index, vk::CommandPoolCreateFlags flags) {
    return vk::CommandPoolCreateInfo{
        .flags = flags,
        .queueFamilyIndex = queue_family_index,
    };
}

vk::CommandBufferAllocateInfo boa::command_buffer_allocate_info(vk::CommandPool pool, uint32_t count, vk::CommandBufferLevel level) {
    return vk::CommandBufferAllocateInfo{
        .commandPool        = pool,
        .level              = level,
        .commandBufferCount = count,
    };
}

vk::PipelineShaderStageCreateInfo boa::pipeline_shader_stage_create_info(vk::ShaderStageFlagBits stage, vk::ShaderModule shader_module) {
    return vk::PipelineShaderStageCreateInfo{
        .stage  = stage,
        .module = shader_module,
        .pName  = "main",
    };
}

vk::PipelineVertexInputStateCreateInfo boa::vertex_input_state_create_info() {
    return vk::PipelineVertexInputStateCreateInfo{
        .vertexBindingDescriptionCount      = 0,
        .vertexAttributeDescriptionCount    = 0,
    };
}

vk::PipelineInputAssemblyStateCreateInfo boa::input_assembly_create_info(vk::PrimitiveTopology topology) {
    return vk::PipelineInputAssemblyStateCreateInfo{
        .topology               = topology,
        .primitiveRestartEnable = false,
    };
}

vk::PipelineRasterizationStateCreateInfo boa::rasterization_state_create_info(vk::PolygonMode polygon_mode) {
    return vk::PipelineRasterizationStateCreateInfo{
        .depthClampEnable           = false,
        .rasterizerDiscardEnable    = false,
        .polygonMode                = polygon_mode,
        //.cullMode                   = vk::CullModeFlagBits::eNone,
        //.frontFace                  = vk::FrontFace::eClockwise,
        .cullMode                   = vk::CullModeFlagBits::eBack,
        .frontFace                  = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable            = false,
        .lineWidth                  = 1.0f,
    };
}

vk::PipelineMultisampleStateCreateInfo boa::multisample_state_create_info() {
    return vk::PipelineMultisampleStateCreateInfo{
        .rasterizationSamples   = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable    = false,
    };
}

vk::PipelineColorBlendAttachmentState boa::color_blend_attachment_state() {
    return vk::PipelineColorBlendAttachmentState{
        .blendEnable    = false,
        .colorWriteMask = vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA,
    };
}

vk::PipelineDepthStencilStateCreateInfo boa::depth_stencil_create_info(bool depth_test, bool depth_write, vk::CompareOp compare_op) {
    return vk::PipelineDepthStencilStateCreateInfo{
        .depthTestEnable        = depth_test,
        .depthWriteEnable       = depth_write,
        .depthCompareOp         = compare_op,
        .depthBoundsTestEnable  = false,
        .stencilTestEnable      = false,
        .front                  = {},
        .back                   = {},
        .minDepthBounds         = 0.0f,
        .maxDepthBounds         = 1.0f,
    };
}

vk::PipelineLayoutCreateInfo boa::pipeline_layout_create_info() {
    return vk::PipelineLayoutCreateInfo{
        .flags                  = vk::PipelineLayoutCreateFlags(),
        .setLayoutCount         = 0,
        .pSetLayouts            = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };
}

vk::ImageCreateInfo boa::image_create_info(vk::Format format, vk::ImageUsageFlags usage, vk::Extent3D extent) {
    return vk::ImageCreateInfo{
        .imageType          = vk::ImageType::e2D,
        .format             = format,
        .extent             = extent,
        .mipLevels          = 1,
        .arrayLayers        = 1,
        .samples            = vk::SampleCountFlagBits::e1,
        .tiling             = vk::ImageTiling::eOptimal,
        .usage              = usage,
        .sharingMode        = vk::SharingMode::eExclusive,
        .initialLayout      = vk::ImageLayout::eUndefined,
    };
}

vk::ImageViewCreateInfo boa::image_view_create_info(vk::Format format, vk::Image image, vk::ImageAspectFlags aspect) {
    return vk::ImageViewCreateInfo{
        .image              = image,
        .viewType           = vk::ImageViewType::e2D,
        .format             = format,
        .subresourceRange   = {
            .aspectMask     = aspect,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };
}

vk::SamplerCreateInfo boa::sampler_create_info(vk::Filter filters, vk::SamplerAddressMode address_mode) {
    return vk::SamplerCreateInfo{
        .magFilter      = filters,
        .minFilter      = filters,
        .mipmapMode     = vk::SamplerMipmapMode::eLinear,
        .addressModeU   = address_mode,
        .addressModeV   = address_mode,
        .addressModeW   = address_mode,
    };
}

vk::WriteDescriptorSet boa::write_descriptor_image(vk::DescriptorType type, vk::DescriptorSet dst, vk::DescriptorImageInfo *image_info, uint32_t binding) {
    return vk::WriteDescriptorSet{
        .dstSet             = dst,
        .dstBinding         = binding,
        .descriptorCount    = 1,
        .descriptorType     = type,
        .pImageInfo         = image_info,
    };
}