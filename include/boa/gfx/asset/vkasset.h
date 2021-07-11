#ifndef BOA_GFX_ASSET_VKASSET_H
#define BOA_GFX_ASSET_VKASSET_H

#include "vk_mem_alloc.h"
#include "glm/glm.hpp"
#include <stdint.h>
#include <vulkan/vulkan.hpp>

namespace boa::gfx {

class Model;
class Renderer;

struct VkMaterial {
    vk::DescriptorSet texture_set{ VK_NULL_HANDLE };
    vk::Pipeline pipeline;
    vk::PipelineLayout pipeline_layout;
};

struct VkTexture {
    VkTexture(Renderer &renderer, const Model::Image &model_image, bool mipmap = true);

    VmaImage image;
    vk::ImageView image_view;
    uint32_t mip_levels;
};

struct VkPrimitive {
    uint32_t index_count;
    uint32_t vertex_offset;
    VmaBuffer index_buffer;
    VkMaterial *material{ nullptr };
    glm::mat4 transform_matrix;
};

struct VkModel {
    VkModel(Renderer &renderer, std::string name, const Model &model);

    std::string name;
    std::vector<VkPrimitive> primitives;
    std::vector<VkTexture> textures;
    std::vector<vk::Sampler> samplers;
    VmaBuffer vertex_buffer;

private:
    Renderer &renderer;

    void add_sampler(const Model::Sampler &sampler);

    void upload_primitive_indices(VkPrimitive &vk_primitive,
        const Model::Primitive &primitive);
    void upload_model_vertices(const Model &model);
};

}

#endif