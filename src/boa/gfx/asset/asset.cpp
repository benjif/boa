#include "boa/macros.h"
#include "boa/gfx/renderer.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/asset.h"
#include "boa/gfx/vk/initializers.h"
#include "glm/gtx/transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "stb_image.h"
#include <cassert>
#include <limits>

namespace boa::gfx {

Skybox::Skybox(Renderer &renderer, const std::array<std::string, 6> &texture_paths)
    : texture(renderer, texture_paths)
{
    vk::DescriptorSetAllocateInfo alloc_info{
        .descriptorPool         = renderer.m_descriptor_pool,
        .descriptorSetCount     = 1,
        .pSetLayouts            = &renderer.m_skybox_set_layout,
    };

    try {
        skybox_set = renderer.m_device.get().allocateDescriptorSets(alloc_info)[0];
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    vk::SamplerCreateInfo sampler_info{
        .magFilter          = vk::Filter::eLinear,
        .minFilter          = vk::Filter::eLinear,
        .mipmapMode         = vk::SamplerMipmapMode::eLinear,
        .addressModeU       = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV       = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW       = vk::SamplerAddressMode::eClampToEdge,
        .anisotropyEnable   = true,
        .maxAnisotropy      = renderer.m_device_properties.limits.maxSamplerAnisotropy,
    };

    vk::Sampler new_sampler;
    try {
        new_sampler = renderer.m_device.get().createSampler(sampler_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create sampler");
    }

    vk::DescriptorImageInfo image_buffer_info{
        .sampler        = new_sampler,
        .imageView      = texture.image_view,
        .imageLayout    = vk::ImageLayout::eShaderReadOnlyOptimal,
    };

    renderer.m_deletion_queue.enqueue([=, device = renderer.m_device.get()]() {
        device.destroySampler(new_sampler);
    });

    vk::WriteDescriptorSet write = write_descriptor_image(vk::DescriptorType::eCombinedImageSampler,
        skybox_set, &image_buffer_info, 0);
    renderer.m_device.get().updateDescriptorSets(write, nullptr);

    sampler = new_sampler;
}

RenderableModel::RenderableModel(AssetManager &asset_manager, Renderer &renderer, const glTFModel &model, LightingInteractivity preferred_lighting)
    : lighting(preferred_lighting),
      root_nodes(model.get_root_nodes().begin(), model.get_root_nodes().end())
{
    nodes.reserve(model.get_node_count());
    primitives.reserve(model.get_primitive_count());

    size_t image_count = model.get_image_count();
    if (image_count >= renderer.MAX_IMAGE_DESCRIPTORS)
        throw std::runtime_error("Bounded more than max image descriptors for set");

    vk::DescriptorSetAllocateInfo alloc_info{
        .descriptorPool         = renderer.m_descriptor_pool,
        .descriptorSetCount     = 1,
        .pSetLayouts            = &renderer.m_textures_set_layout,
    };

    try {
        m_textures_descriptor_set = renderer.m_device.get().allocateDescriptorSets(alloc_info)[0];
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to allocate descriptor sets");
    }

    model.for_each_node([&](const auto &node) {
        add_from_node(asset_manager, renderer, model, node);
        return Iteration::Continue;
    });

    upload_model_vertices(renderer, model);

    bounding_box.min = glm::vec3(std::numeric_limits<float>::max());
    bounding_box.max = glm::vec3(std::numeric_limits<float>::min());
    model.for_each_root_node([&](const auto &root_node) {
        calculate_model_bounding_box(model, root_node, glm::mat4{ 1.0f });
        return Iteration::Continue;
    });

    upload_bounding_box_vertices(renderer);
}

static inline vk::Filter tinygltf_to_vulkan_filter(int gltf) {
    switch (gltf) {
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return vk::Filter::eNearest;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
    default:
        return vk::Filter::eLinear;
    }
}

static inline vk::SamplerAddressMode tinygltf_to_vulkan_address_mode(int gltf) {
    switch (gltf) {
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return vk::SamplerAddressMode::eClampToEdge;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return vk::SamplerAddressMode::eMirroredRepeat;
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return vk::SamplerAddressMode::eRepeat;
    default:
        return vk::SamplerAddressMode::eRepeat;
    }
}

vk::Sampler RenderableModel::create_sampler(Renderer &renderer, const glTFModel::Sampler &sampler) {
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

    renderer.m_deletion_queue.enqueue([=, device = renderer.m_device.get()]() {
        device.destroySampler(new_sampler);
    });

    return new_sampler;
}

void RenderableModel::calculate_model_bounding_box(const glTFModel &model, const glTFModel::Node &node, glm::mat4 transform_matrix) {
    transform_matrix *= glm::mat4(node.matrix);

    if (node.mesh.has_value()) {
        const auto &mesh = model.get_mesh(node.mesh.value());

        for (size_t primitive_idx : mesh.primitives) {
            const auto &primitive = model.get_primitive(primitive_idx);

            bounding_box.min = glm::min(glm::vec3(transform_matrix * glm::vec4(primitive.bounding_box.min, 1.f)), bounding_box.min);
            bounding_box.max = glm::max(glm::vec3(transform_matrix * glm::vec4(primitive.bounding_box.max, 1.f)), bounding_box.max);
        }
    }

    for (size_t child_idx : node.children)
        calculate_model_bounding_box(model, model.get_node(child_idx), transform_matrix);
}

void RenderableModel::add_from_node(AssetManager &asset_manager, Renderer &renderer, const glTFModel &model, const glTFModel::Node &node) {
    Node new_boa_node;
    new_boa_node.children.reserve(node.children.size());
    new_boa_node.transform_matrix = node.matrix;
    new_boa_node.id = node.id;

    for (size_t child : node.children)
        new_boa_node.children.push_back(child);

    if (node.mesh.has_value()) {
        const auto &mesh = model.get_mesh(node.mesh.value());
        new_boa_node.children.reserve(mesh.primitives.size());

        for (size_t primitive_idx : mesh.primitives) {
            const auto &primitive = model.get_primitive(primitive_idx);
            Primitive new_boa_primitive;

            if (primitive.material.has_value()) {
                const auto &material = model.get_material(primitive.material.value());
                if (model.get_material(primitive.material.value()).metallic_roughness.base_color_texture.has_value()) {
                    if (material.metallic_roughness.base_color_texture.has_value()) {
                        const auto &base_texture = model.get_texture(material.metallic_roughness.base_color_texture.value());

                        if (base_texture.sampler.has_value() && base_texture.source.has_value()) {
                            const auto &image = model.get_image(base_texture.source.value());

                            size_t material_index = 0;
                            switch (lighting) {
                            case LightingInteractivity::BlinnPhong:
                                material_index = renderer.TEXTURED_BLINN_PHONG_MATERIAL_INDEX;
                                break;
                            case LightingInteractivity::Unlit:
                            default:
                                material_index = renderer.TEXTURED_MATERIAL_INDEX;
                                break;
                            }

                            Material &base_material = asset_manager.get_material(material_index);

                            uint32_t new_material_index = asset_manager.create_material(base_material.pipeline, base_material.pipeline_layout);
                            Material &new_material = asset_manager.get_material(new_material_index);

                            new_material.texture_set = m_textures_descriptor_set;
                            new_material.descriptor_number = m_descriptor_count;
                            new_material.base_color = glm::make_vec4(material.metallic_roughness.base_color_factor.data());
                            new_material.color_type = Material::ColorType::Texture;

                            Texture new_texture(renderer, image);
                            vk::Sampler new_sampler = create_sampler(renderer, model.get_sampler(base_texture.sampler.value()));

                            vk::DescriptorImageInfo image_buffer_info{
                                .sampler        = new_sampler,
                                .imageView      = new_texture.image_view,
                                .imageLayout    = vk::ImageLayout::eShaderReadOnlyOptimal,
                            };

                            vk::WriteDescriptorSet write = write_descriptor_image(vk::DescriptorType::eCombinedImageSampler,
                                new_material.texture_set, &image_buffer_info, 0);
                            write.dstArrayElement = m_descriptor_count;
                            renderer.m_device.get().updateDescriptorSets(write, nullptr);

                            new_boa_primitive.material = new_material_index;
                            m_descriptor_count++;
                        }
                    }
                } else {
                    size_t material_index = 0;
                    switch (lighting) {
                    case LightingInteractivity::BlinnPhong:
                        material_index = renderer.UNTEXTURED_BLINN_PHONG_MATERIAL_INDEX;
                        break;
                    case LightingInteractivity::Unlit:
                    default:
                        material_index = renderer.UNTEXTURED_MATERIAL_INDEX;
                        break;
                    }

                    Material &base_material = asset_manager.get_material(material_index);

                    uint32_t new_material_index = asset_manager.create_material(base_material.pipeline, base_material.pipeline_layout);
                    Material &new_material = asset_manager.get_material(new_material_index);

                    if (primitive.has_vertex_coloring) {
                        new_material.color_type = Material::ColorType::Vertex;
                    } else {
                        new_material.color_type = Material::ColorType::Base;
                        new_material.base_color = glm::make_vec4(material.metallic_roughness.base_color_factor.data());
                    }

                    new_boa_primitive.material = new_material_index;
                }
            }

            new_boa_primitive.index_count = primitive.indices.size();
            new_boa_primitive.bounding_sphere = primitive.bounding_sphere;

            upload_primitive_indices(renderer, new_boa_primitive, primitive);

            new_boa_node.primitives.push_back(primitives.size());
            primitives.push_back(std::move(new_boa_primitive));
        }
    }

    nodes.push_back(std::move(new_boa_node));
}

void Texture::init(Renderer &renderer, uint32_t w, uint32_t h, void *img_data, bool mipmap) {
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

    vk::ImageViewCreateInfo view_info{
        .image              = new_image.image,
        .viewType           = vk::ImageViewType::e2D,
        .format             = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange   = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = image_mip_levels,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },
    };

    vk::ImageView new_image_view;
    try {
        new_image_view = renderer.m_device.get().createImageView(view_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create image view");
    }

    renderer.m_deletion_queue.enqueue([=, &allocator = renderer.m_allocator, &device = renderer.m_device]() {
        vmaDestroyImage(allocator, new_image.image, new_image.allocation);
        device.get().destroyImageView(new_image_view);
    });

    vmaDestroyBuffer(renderer.m_allocator, staging_buffer.buffer, staging_buffer.allocation);

    image = new_image;
    image_view = new_image_view;
    mip_levels = image_mip_levels;
}

Texture::Texture(Renderer &renderer, const char *path, bool mipmap) {
    int w, h, channels;
    stbi_uc *pixels = stbi_load(path, &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels)
        throw std::runtime_error("Failed to load texture file");

    init(std::forward<Renderer &>(renderer), w, h, pixels, mipmap);

    stbi_image_free(pixels);
}

Texture::Texture(Renderer &renderer, const glTFModel::Image &model_image, bool mipmap) {
    init(std::forward<Renderer &>(renderer), model_image.width, model_image.height, model_image.data, mipmap);
}

Texture::Texture(Renderer &renderer, const std::array<std::string, 6> &texture_paths) {
    int w[6], h[6], channels[6];
    stbi_uc *pixels[6];

    for (size_t i = 0; i < 6; i++) {
        pixels[i] = stbi_load(texture_paths[i].c_str(), &w[i], &h[i], &channels[i], STBI_rgb_alpha);
        assert(w[i] == w[0] && h[i] == h[0]);
        if (!pixels[i])
            throw std::runtime_error("Failed to load texture file (skybox)");
    }

    vk::DeviceSize image_size = w[0] * h[0] * 6 * 4;
    vk::DeviceSize layer_size = image_size / 6;

    VmaBuffer staging_buffer = renderer.create_buffer(image_size, vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer.m_allocator, staging_buffer.allocation, &data);

    for (size_t i = 0; i < 6; i++)
        memcpy(static_cast<void *>(static_cast<char *>(data) + layer_size * i), pixels[i], layer_size);

    vmaUnmapMemory(renderer.m_allocator, staging_buffer.allocation);

    vk::Extent3D image_extent{
        .width  = (uint32_t)w[0],
        .height = (uint32_t)h[0],
        .depth  = 1,
    };

    vk::ImageUsageFlags image_usage_flags = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;

    vk::ImageCreateInfo image_info = image_create_info(vk::Format::eR8G8B8A8Srgb,
        image_usage_flags, image_extent, 1);
    image_info.flags = vk::ImageCreateFlagBits::eCubeCompatible;
    image_info.arrayLayers = 6;

    VmaImage new_image;

    VmaAllocationCreateInfo image_alloc_info{ .usage = VMA_MEMORY_USAGE_GPU_ONLY };
    vmaCreateImage(renderer.m_allocator, (VkImageCreateInfo *)&image_info, &image_alloc_info, (VkImage *)&new_image.image,
        &new_image.allocation, nullptr);

    renderer.immediate_command([&](vk::CommandBuffer cmd) {
        vk::ImageSubresourceRange range{
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 6,
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
                .layerCount     = 6,
            },
            .imageExtent        = {
                .width          = (uint32_t)w[0],
                .height         = (uint32_t)h[0],
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

    vk::ImageViewCreateInfo view_info{
        .image              = new_image.image,
        .viewType           = vk::ImageViewType::eCube,
        .format             = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange   = {
            .aspectMask     = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 6,
        },
    };

    vk::ImageView new_image_view;
    try {
        new_image_view = renderer.m_device.get().createImageView(view_info);
    } catch (const vk::SystemError &err) {
        throw std::runtime_error("Failed to create image view");
    }

    renderer.m_deletion_queue.enqueue([=, allocator = renderer.m_allocator, device = renderer.m_device.get()]() {
        vmaDestroyImage(allocator, new_image.image, new_image.allocation);
        device.destroyImageView(new_image_view);
    });

    vmaDestroyBuffer(renderer.m_allocator, staging_buffer.buffer, staging_buffer.allocation);

    image = new_image;
    image_view = new_image_view;

    for (size_t i = 0; i < 6; i++)
        stbi_image_free(pixels[i]);
}

void RenderableModel::upload_primitive_indices(Renderer &renderer, Primitive &vk_primitive, const glTFModel::Primitive &primitive) {
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

void RenderableModel::upload_model_vertices(Renderer &renderer, const glTFModel &model) {
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

    renderer.m_deletion_queue.enqueue([copy = vertex_buffer, &allocator = renderer.m_allocator]() {
        vmaDestroyBuffer(allocator, copy.buffer,
            copy.allocation);
    });

    vmaDestroyBuffer(renderer.m_allocator, staging_buffer.buffer, staging_buffer.allocation);
}

void RenderableModel::upload_bounding_box_vertices(Renderer &renderer) {
    std::array<Vertex, 8> bounding_box_vertices{
        Vertex{ .position = { bounding_box.min }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
        Vertex{ .position = { bounding_box.max }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
        Vertex{ .position = { bounding_box.min.x, bounding_box.min.y, bounding_box.max.z }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
        Vertex{ .position = { bounding_box.min.x, bounding_box.max.y, bounding_box.min.z }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
        Vertex{ .position = { bounding_box.max.x, bounding_box.min.y, bounding_box.min.z }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
        Vertex{ .position = { bounding_box.min.x, bounding_box.max.y, bounding_box.max.z }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
        Vertex{ .position = { bounding_box.max.x, bounding_box.min.y, bounding_box.max.z }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
        Vertex{ .position = { bounding_box.max.x, bounding_box.max.y, bounding_box.min.z }, .color0 = { 1.0f, 0.0f, 0.0f, 1.0f }},
    };

    std::array<Vertex, 24> bounding_box_vertices_repeated{
        bounding_box_vertices[5], bounding_box_vertices[1],
        bounding_box_vertices[1], bounding_box_vertices[7],
        bounding_box_vertices[7], bounding_box_vertices[3],
        bounding_box_vertices[3], bounding_box_vertices[5],
        bounding_box_vertices[2], bounding_box_vertices[6],
        bounding_box_vertices[6], bounding_box_vertices[4],
        bounding_box_vertices[4], bounding_box_vertices[0],
        bounding_box_vertices[0], bounding_box_vertices[2],
        bounding_box_vertices[5], bounding_box_vertices[2],
        bounding_box_vertices[1], bounding_box_vertices[6],
        bounding_box_vertices[7], bounding_box_vertices[4],
        bounding_box_vertices[3], bounding_box_vertices[0],
    };

    const size_t size = bounding_box_vertices_repeated.size() * sizeof(Vertex);

    VmaBuffer staging_buffer = renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(renderer.m_allocator, staging_buffer.allocation, &data);
    memcpy(data, bounding_box_vertices_repeated.data(), size);
    vmaUnmapMemory(renderer.m_allocator, staging_buffer.allocation);

    bounding_box_vertex_buffer = renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY);

    renderer.immediate_command([=](vk::CommandBuffer cmd) {
        vk::BufferCopy copy{ .srcOffset = 0, .dstOffset = 0, .size = size };
        cmd.copyBuffer(staging_buffer.buffer, bounding_box_vertex_buffer.buffer, copy);
    });

    renderer.m_deletion_queue.enqueue([copy = bounding_box_vertex_buffer, &allocator = renderer.m_allocator]() {
        vmaDestroyBuffer(allocator, copy.buffer,
            copy.allocation);
    });

    vmaDestroyBuffer(renderer.m_allocator, staging_buffer.buffer, staging_buffer.allocation);
}

}
