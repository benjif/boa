#ifndef BOA_GFX_ASSET_VKASSET_H
#define BOA_GFX_ASSET_VKASSET_H

#include "vk_mem_alloc.h"
#include "glm/glm.hpp"
#include <stdint.h>
#include <vulkan/vulkan.hpp>

namespace boa::gfx {

class glTFModel;
class Renderer;

struct VkMaterial {
    std::string name;
    uint32_t descriptor_number;
    vk::DescriptorSet texture_set{ VK_NULL_HANDLE };
    vk::Pipeline pipeline{ VK_NULL_HANDLE };
    vk::PipelineLayout pipeline_layout{ VK_NULL_HANDLE };
};

struct VkTexture {
    VkTexture(Renderer &renderer, const glTFModel::Image &model_image, bool mipmap = true);
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
    size_t material;

    Sphere bounding_sphere;
    glm::mat4 transform_matrix;
};

struct VkNode {
    std::vector<size_t> primitives;
    std::vector<size_t> children;
    glm::mat4 transform_matrix;
};

struct VkModel {
    VkModel(Renderer &renderer, const std::string &model_name, const glTFModel &model_model);

    std::string name;
    std::vector<VkNode> nodes;
    std::vector<VkPrimitive> primitives;
    std::vector<VkTexture> textures;
    std::vector<vk::Sampler> samplers;

    std::vector<size_t> root_nodes;
    size_t root_node_count{ 0 };

    VmaBuffer vertex_buffer;

private:
    uint32_t descriptor_count{ 0 };
    vk::DescriptorSet textures_descriptor_set;
    vk::DescriptorSetLayout textures_descriptor_set_layout;

    Renderer &renderer;
    const glTFModel &model;

    void add_sampler(const glTFModel::Sampler &sampler);
    void add_from_node(const glTFModel::Node &node);

    void upload_primitive_indices(VkPrimitive &vk_primitive,
        const glTFModel::Primitive &primitive);
    void upload_model_vertices(const glTFModel &model);
};

}

#endif