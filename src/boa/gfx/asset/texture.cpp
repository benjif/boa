#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include "boa/gfx/asset/texture.h"
#include "boa/gfx/vk/initializers.h"
#include "boa/gfx/renderer.h"
#include "vk_mem_alloc.h"
#include <algorithm>

namespace boa::gfx {

void Texture::load_from_file(Renderer *renderer, const char *path, bool mipmap) {
    if (!renderer)
        throw std::runtime_error("Attempt to load texture with null renderer pointer");

    int w, h, channels;
    stbi_uc *pixels = stbi_load(path, &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels)
        throw std::runtime_error("Failed to load texture");

    uint32_t image_mip_levels = 1;
    if (mipmap) {
        if (!(renderer->m_device_format_properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
            throw std::runtime_error("Requested mipmapping when the physical device doesn't support linear filtering");
        image_mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1;
    }

    vk::DeviceSize image_size = w * h * 4;

    VmaBuffer staging_buffer = renderer->create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer->m_allocator, staging_buffer.allocation, &data);
    memcpy(data, (void *)pixels, (size_t)image_size);
    vmaUnmapMemory(renderer->m_allocator, staging_buffer.allocation);

    stbi_image_free(pixels);

    vk::Extent3D image_extent{
        .width  = (uint32_t)w,
        .height = (uint32_t)h,
        .depth  = 1,
    };

    vk::ImageUsageFlags image_usage_flags = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    if (mipmap)
        image_usage_flags |= vk::ImageUsageFlagBits::eTransferSrc;

    vk::ImageCreateInfo image_info = image_create_info(vk::Format::eR8G8B8A8Srgb,
        image_usage_flags, image_extent, image_mip_levels);

    VmaImage new_image;

    VmaAllocationCreateInfo image_alloc_info{ .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vmaCreateImage(renderer->m_allocator, (VkImageCreateInfo *)&image_info, &image_alloc_info, (VkImage *)&new_image.image,
        &new_image.allocation, nullptr);

    renderer->immediate_command([&](vk::CommandBuffer cmd) {
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
                .width          = (uint32_t)w,
                .height         = (uint32_t)h,
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

    vk::ImageView new_image_view = renderer->create_image_view(new_image.image, vk::Format::eR8G8B8A8Srgb,
        vk::ImageAspectFlagBits::eColor, image_mip_levels);

    renderer->m_deletion_queue.enqueue([=]() {
        vmaDestroyImage(renderer->m_allocator, new_image.image, new_image.allocation);
        renderer->m_device.get().destroyImageView(new_image_view);
    });

    vmaDestroyBuffer(renderer->m_allocator, staging_buffer.buffer, staging_buffer.allocation);

    image = new_image;
    image_view = new_image_view;
    mip_levels = image_mip_levels;
}

}