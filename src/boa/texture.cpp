#include "boa/texture.h"
#include "boa/initializers.h"
#include "boa/renderer.h"
#include "stb_image.h"
#include "vk_mem_alloc.h"
#include <algorithm>

boa::Texture::Texture() {}

void boa::Texture::load_from_file(Renderer *renderer, const char *path) {
    if (!renderer)
        throw std::runtime_error("Attempt to load texture with null renderer pointer");

    int w, h, channels;
    stbi_uc *pixels = stbi_load(path, &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels)
        throw std::runtime_error("Failed to load texture");

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

    vk::ImageCreateInfo image_info = image_create_info(vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst, image_extent);

    VmaImage new_image;

    VmaAllocationCreateInfo image_alloc_info{ .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vmaCreateImage(renderer->m_allocator, (VkImageCreateInfo *)&image_info, &image_alloc_info, (VkImage *)&new_image.image,
        &new_image.allocation, nullptr);

    renderer->immediate_command([&](vk::CommandBuffer cmd) {
        vk::ImageSubresourceRange range{
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
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

    vk::ImageView new_image_view = renderer->create_image_view(new_image.image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

    renderer->m_deletion_queue.enqueue([=]() {
        vmaDestroyImage(renderer->m_allocator, new_image.image, new_image.allocation);
        renderer->m_device.get().destroyImageView(new_image_view);
    });

    vmaDestroyBuffer(renderer->m_allocator, staging_buffer.buffer, staging_buffer.allocation);

    image = new_image;
    image_view = new_image_view;
}