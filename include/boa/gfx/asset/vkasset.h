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
    uint32_t descriptor_number;
    vk::DescriptorSet texture_set{ VK_NULL_HANDLE };
    vk::Pipeline pipeline;
    vk::PipelineLayout pipeline_layout;
};

struct VkTexture {
    VkTexture(Renderer &renderer, const Model::Image &model_image, bool mipmap = true);
    VkTexture(Renderer &renderer, const char *path, bool mipmap = true);

    VmaImage image;
    vk::ImageView image_view;
    uint32_t mip_levels;

private:
    void init(Renderer &renderer, uint32_t w, uint32_t h, void *img_data, bool mipmap);
};

struct VkPrimitive {
    uint32_t index_count;
    VmaBuffer index_buffer;

    VkMaterial *material{ nullptr };

    Sphere bounding_sphere;
    glm::mat4 transform_matrix;
};

struct VkModel {
    VkModel(Renderer &renderer, const std::string &model_name, const Model &model_model);

    std::string name;
    std::vector<vk::Sampler> samplers;
    std::vector<VkTexture> textures;
    std::vector<VkPrimitive> primitives;
    VmaBuffer vertex_buffer;

private:
    uint32_t descriptor_count{ 0 };
    vk::DescriptorSet textures_descriptor_set;
    vk::DescriptorSetLayout textures_descriptor_set_layout;

    Renderer &renderer;
    const Model &model;

    void add_sampler(const Model::Sampler &sampler);
    void add_from_node(const Model::Node &node);

    void upload_primitive_indices(VkPrimitive &vk_primitive,
        const Model::Primitive &primitive);
    void upload_model_vertices(const Model &model);
};

}

#endif