#include "boa/macros.h"
#include "boa/gfx/renderer.h"
#include "boa/gfx/asset/model.h"
#include "boa/gfx/asset/vkasset.h"
#include "boa/gfx/vk/initializers.h"
#include "stb_image.h"

namespace boa::gfx {

VkModel::VkModel(Renderer &renderer, const std::string &model_name, const Model &model_model)
    : name(std::move(model_name)),
      renderer(renderer),
      model(model_model)
{
    LOG_INFO("(Asset) Loading model '{}'", name);

    primitives.reserve(model.get_primitive_count());
    textures.reserve(model.get_texture_count());
    samplers.reserve(model.get_sampler_count());

    size_t image_count = model.get_image_count();

    vk::DescriptorSetAllocateInfo alloc_info{
        .descriptorPool         = renderer.m_descriptor_pool,
        .descriptorSetCount     = 1,
        .pSetLayouts            = &renderer.m_texture_descriptor_set_layout,
    };

    try {
        texture_descriptor_set = renderer.m_device.get().allocateDescriptorSets(alloc_info)[0];
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    model.for_each_sampler([&](const auto &sampler) {
        add_sampler(sampler);
        return Iteration::Continue;
    });

    model.for_each_node([&](const auto &node) {
        add_from_node(node);
        return Iteration::Continue;
    });

    upload_model_vertices(model);
}

static inline vk::Filter tinygltf_to_vulkan_filter(int gltf) {
    switch (gltf) {
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        return vk::Filter::eLinear;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return vk::Filter::eNearest;
    default:
        return vk::Filter::eLinear;
    }
}

static inline vk::SamplerAddressMode tinygltf_to_vulkan_address_mode(int gltf) {
    switch (gltf) {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return vk::SamplerAddressMode::eRepeat;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return vk::SamplerAddressMode::eClampToEdge;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return vk::SamplerAddressMode::eMirroredRepeat;
    default:
        return vk::SamplerAddressMode::eRepeat;
    }
}

void VkModel::add_sampler(const Model::Sampler &sampler) {
    vk::SamplerCreateInfo sampler_info{
        .magFilter          = tinygltf_to_vulkan_filter(sampler.mag_filter),
        .minFilter          = tinygltf_to_vulkan_filter(sampler.min_filter),
        .mipmapMode         = vk::SamplerMipmapMode::eLinear,
        .addressModeU       = tinygltf_to_vulkan_address_mode(sampler.wrap_s_mode),
        .addressModeV       = tinygltf_to_vulkan_address_mode(sampler.wrap_t_mode),
        .anisotropyEnable   = true,
        .maxAnisotropy      = renderer.m_device_properties.limits.maxSamplerAnisotropy,
    };

    vk::Sampler new_sampler;
    try {
        new_sampler = renderer.m_device.get().createSampler(sampler_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create sampler");
    }

    samplers.push_back(new_sampler);

    renderer.m_deletion_queue.enqueue([=, device = renderer.m_device.get()]() {
        device.destroySampler(new_sampler);
    });
}

void VkModel::add_from_node(const Model::Node &node) {
    glm::mat4 transform;

    if (node.mesh.has_value()) {
        const auto &mesh = model.get_mesh(node.mesh.value());
        for (size_t primitive_idx : mesh.primitives) {
            const auto &primitive = model.get_primitive(primitive_idx);
            VkPrimitive new_vk_primitive;
            new_vk_primitive.material = renderer.get_material("untextured");

            if (primitive.material.has_value()) {
                const auto &material = model.get_material(primitive.material.value());
                if (material.metallic_roughness.base_color_texture.has_value()) {
                    const auto &base_texture = model.get_texture(material.metallic_roughness.base_color_texture.value());
                    if (base_texture.sampler.has_value() && base_texture.source.has_value()) {
                        const auto &sampler = model.get_sampler(base_texture.sampler.value());
                        const auto &image = model.get_image(base_texture.source.value());

                        VkTexture new_texture(renderer, image);
                        textures.push_back(std::move(new_texture));

                        VkMaterial *textured = renderer.get_material("textured");
                        VkMaterial *new_textured = renderer.create_material(textured->pipeline, textured->pipeline_layout,
                            fmt::format("textured_{}_{}", name, primitive_idx));

                        LOG_INFO("(Asset) Creating new material 'textured_{}_{}'", name, primitive_idx);

                        new_textured->texture_set = texture_descriptor_set;

                        vk::DescriptorImageInfo image_buffer_info{
                            .sampler        = samplers[base_texture.sampler.value()],
                            .imageView      = textures.back().image_view,
                            .imageLayout    = vk::ImageLayout::eShaderReadOnlyOptimal,
                        };

                        vk::WriteDescriptorSet write = write_descriptor_image(vk::DescriptorType::eCombinedImageSampler,
                            new_textured->texture_set, &image_buffer_info, 0);
                        renderer.m_device.get().updateDescriptorSets(write, 0);

                        new_vk_primitive.material = new_textured;
                    }
                }
            }

            new_vk_primitive.index_count = primitive.indices.size();
            new_vk_primitive.vertex_offset = primitive.vertex_offset;
            new_vk_primitive.transform_matrix = node.matrix;

            upload_primitive_indices(new_vk_primitive, primitive);

            primitives.push_back(std::move(new_vk_primitive));
        }
    }
}

void VkTexture::init(Renderer &renderer, uint32_t w, uint32_t h, void *img_data, bool mipmap) {
    uint32_t image_mip_levels = 1;
    if (mipmap) {
        if (!(renderer.m_device_format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
            throw std::runtime_error("Requested mipmapping when the physical device doesn't support linear filtering");
        image_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;
    }

    vk::DeviceSize image_size = w * h * 4;

    VmaBuffer staging_buffer = renderer.create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer.m_allocator, staging_buffer.allocation, &data);
    memcpy(data, (void *)img_data, (size_t)image_size);
    vmaUnmapMemory(renderer.m_allocator, staging_buffer.allocation);

    vk::Extent3D image_extent{
        .width  = w,
        .height = h,
        .depth  = 1,
    };

    vk::ImageUsageFlags image_usage_flags = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    if (mipmap)
        image_usage_flags |= vk::ImageUsageFlagBits::eTransferSrc;

    vk::ImageCreateInfo image_info = image_create_info(vk::Format::eR8G8B8A8Srgb,
        image_usage_flags, image_extent, image_mip_levels);

    VmaImage new_image;

    VmaAllocationCreateInfo image_alloc_info{ .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vmaCreateImage(renderer.m_allocator, (VkImageCreateInfo *)&image_info, &image_alloc_info, (VkImage *)&new_image.image,
        &new_image.allocation, nullptr);

    renderer.immediate_command([&](vk::CommandBuffer cmd) {
        vk::ImageSubresourceRange range{
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = image_mip_levels,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        };

        vk::ImageMemoryBarrier image_barrier_to_transfer{
            .srcAccessMask      = vk::AccessFlagBits::eNoneKHR,
            .dstAccessMask      = vk::AccessFlagBits::eTransferWrite,
            .oldLayout          = vk::ImageLayout::eUndefined,
            .newLayout          = vk::ImageLayout::eTransferDstOptimal,
            .image              = new_image.image,
            .subresourceRange   = range,
        };

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlags{},
            nullptr,
            nullptr,
            image_barrier_to_transfer);

        vk::BufferImageCopy copy{
            .bufferOffset       = 0,
            .bufferRowLength    = 0,
            .bufferImageHeight  = 0,
            .imageSubresource   = {
                .aspectMask     = vk::ImageAspectFlagBits::eColor,
                .mipLevel       = 0,
                .baseArrayLayer = 0,
                .layerCount     = 1,
            },
            .imageExtent        = {
                .width          = w,
                .height         = h,
                .depth          = 1,
            },
        };

        cmd.copyBufferToImage(
            staging_buffer.buffer,
            new_image.image,
            vk::ImageLayout::eTransferDstOptimal,
            copy);

        if (mipmap) {
            vk::ImageSubresourceRange mipmap_range = range;
            mipmap_range.levelCount = 1;

            vk::ImageMemoryBarrier mipmap_barrier{
                .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
                .image                  = new_image.image,
                .subresourceRange       = mipmap_range,
            };

            int32_t mip_w = w, mip_h = h;
            for (uint32_t i = 1; i < image_mip_levels; i++) {
                mipmap_barrier.subresourceRange.baseMipLevel = i - 1;
                mipmap_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
                mipmap_barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
                mipmap_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                mipmap_barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

                cmd.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::DependencyFlags{},
                    nullptr,
                    nullptr,
                    mipmap_barrier);

                const std::array<vk::Offset3D, 2> src_offsets = {
                    vk::Offset3D{ 0, 0, 0 },
                    vk::Offset3D{ mip_w, mip_h, 1 },
                };

                const std::array<vk::Offset3D, 2> dst_offsets = {
                    vk::Offset3D{ 0, 0, 0 },
                    vk::Offset3D{ mip_w > 1 ? mip_w / 2 : 1, mip_h > 1 ? mip_h / 2 : 1, 1 },
                };

                vk::ImageBlit blit{
                    .srcSubresource     = {
                        .aspectMask     = vk::ImageAspectFlagBits::eColor,
                        .mipLevel       = i - 1,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                    .srcOffsets         = src_offsets,
                    .dstSubresource     = {
                        .aspectMask     = vk::ImageAspectFlagBits::eColor,
                        .mipLevel       = i,
                        .baseArrayLayer = 0,
                        .layerCount     = 1,
                    },
                    .dstOffsets         = dst_offsets,
                };

                cmd.blitImage(
                    new_image.image,
                    vk::ImageLayout::eTransferSrcOptimal,
                    new_image.image,
                    vk::ImageLayout::eTransferDstOptimal,
                    blit,
                    vk::Filter::eLinear);

                mipmap_barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
                mipmap_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                mipmap_barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
                mipmap_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

                cmd.pipelineBarrier(
                    vk::PipelineStageFlagBits::eTransfer,
                    vk::PipelineStageFlagBits::eFragmentShader,
                    vk::DependencyFlags{},
                    nullptr,
                    nullptr,
                    mipmap_barrier);

                if (mip_w > 1)
                    mip_w /= 2;
                if (mip_h > 1)
                    mip_h /= 2;
            }

            mipmap_barrier.subresourceRange.baseMipLevel = image_mip_levels - 1;
            mipmap_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
            mipmap_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            mipmap_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            mipmap_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eFragmentShader,
                vk::DependencyFlags{},
                nullptr,
                nullptr,
                mipmap_barrier);

            return;
        }

        vk::ImageMemoryBarrier image_barrier_to_readable{
            .srcAccessMask      = vk::AccessFlagBits::eTransferWrite,
            .dstAccessMask      = vk::AccessFlagBits::eShaderRead,
            .oldLayout          = vk::ImageLayout::eTransferDstOptimal,
            .newLayout          = vk::ImageLayout::eShaderReadOnlyOptimal,
            .image              = new_image.image,
            .subresourceRange   = range,
        };

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlags{},
            nullptr,
            nullptr,
            image_barrier_to_readable);
    });

    vk::ImageView new_image_view = renderer.create_image_view(new_image.image, vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor, image_mip_levels);

    renderer.m_deletion_queue.enqueue([=, &allocator = renderer.m_allocator, &device = renderer.m_device]() {
        vmaDestroyImage(allocator, new_image.image, new_image.allocation);
        device.get().destroyImageView(new_image_view);
    });

    vmaDestroyBuffer(renderer.m_allocator, staging_buffer.buffer, staging_buffer.allocation);

    image = new_image;
    image_view = new_image_view;
    mip_levels = image_mip_levels;
}

VkTexture::VkTexture(Renderer &renderer, const char *path, bool mipmap) {
    int w, h, channels;
    stbi_uc *pixels = stbi_load(path, &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels)
        throw std::runtime_error("Failed to load texture file");

    init(std::forward<Renderer &>(renderer), w, h, pixels, mipmap);
}

VkTexture::VkTexture(Renderer &renderer, const Model::Image &model_image, bool mipmap) {
    init(std::forward<Renderer &>(renderer), model_image.width, model_image.height, model_image.data, mipmap);
}

void VkModel::upload_primitive_indices(VkPrimitive &vk_primitive, const Model::Primitive &primitive) {
    const size_t size = primitive.indices.size() * sizeof(uint32_t);

    VmaBuffer staging_buffer = renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer.m_allocator, staging_buffer.allocation, &data);
    memcpy(data, primitive.indices.data(), size);
    vmaUnmapMemory(renderer.m_allocator, staging_buffer.allocation);

    vk_primitive.index_buffer = renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY);

    renderer.immediate_command([=](vk::CommandBuffer cmd) {
        vk::BufferCopy copy{ .srcOffset = 0, .dstOffset = 0, .size = size };
        cmd.copyBuffer(staging_buffer.buffer, vk_primitive.index_buffer.buffer, copy);
    });

    renderer.m_deletion_queue.enqueue([=, &allocator = renderer.m_allocator]() {
        vmaDestroyBuffer(allocator, vk_primitive.index_buffer.buffer, vk_primitive.index_buffer.allocation);
    });

    vmaDestroyBuffer(renderer.m_allocator, staging_buffer.buffer, staging_buffer.allocation);
}

void VkModel::upload_model_vertices(const Model &model) {
    const size_t size = model.get_vertices().size() * sizeof(Vertex);

    VmaBuffer staging_buffer = renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer.m_allocator, staging_buffer.allocation, &data);
    memcpy(data, model.get_vertices().data(), size);
    vmaUnmapMemory(renderer.m_allocator, staging_buffer.allocation);

    vertex_buffer = renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY);

    renderer.immediate_command([=](vk::CommandBuffer cmd) {
        vk::BufferCopy copy{ .srcOffset = 0, .dstOffset = 0, .size = size };
        cmd.copyBuffer(staging_buffer.buffer, vertex_buffer.buffer, copy);
    });

    /*renderer.m_deletion_queue.enqueue([=, &allocator = renderer.m_allocator]() {
        vmaDestroyBuffer(allocator, vertex_buffer.buffer, vertex_buffer.allocation);
    });*/

    vmaDestroyBuffer(renderer.m_allocator, staging_buffer.buffer, staging_buffer.allocation);
}

}