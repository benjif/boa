#ifndef BOA_GFX_ASSET_ASSET_H
#define BOA_GFX_ASSET_ASSET_H

#include "boa/gfx/linear.h"
#include "boa/gfx/vk/types.h"
#include "boa/gfx/lighting_type.h"
#include "boa/gfx/asset/gltf_model.h"
#include "glm/glm.hpp"
#include <array>
#include <string>
#include <vulkan/vulkan.hpp>

namespace boa::gfx {

class glTFModel;
class AssetManager;
class Renderer;

struct Material {
    std::string name;
    uint32_t descriptor_number;
    vk::DescriptorSet texture_set{ VK_NULL_HANDLE };
    vk::Pipeline pipeline{ VK_NULL_HANDLE };
    vk::PipelineLayout pipeline_layout{ VK_NULL_HANDLE };
};

struct Texture {
    Texture(Renderer &renderer, const glTFModel::Image &model_image, bool mipmap = true);
    Texture(Renderer &renderer, const char *path, bool mipmap = true);
    Texture(Renderer &renderer, const std::array<std::string, 6> &texture_paths);

    VmaImage image;
    vk::ImageView image_view;
    uint32_t mip_levels;

private:
    void init(Renderer &renderer, uint32_t w, uint32_t h, void *img_data, bool mipmap);
};

struct Skybox {
    Skybox(Renderer &renderer, const std::array<std::string, 6> &texture_paths);

    vk::DescriptorSet skybox_set{ VK_NULL_HANDLE };
    vk::Sampler sampler;
    Texture texture;
};

struct Primitive {
    uint32_t index_count;
    VmaBuffer index_buffer;
    size_t material;

    Sphere bounding_sphere;
};

struct Node {
    size_t id;
    std::vector<size_t> primitives;
    std::vector<size_t> children;
    glm::mat4 transform_matrix;
};

struct RenderableModel {
    RenderableModel(AssetManager &asset_manager, Renderer &renderer, const std::string &model_name,
        const glTFModel &model_model, LightingInteractivity preferred_lighting);

    std::string name;
    std::vector<Node> nodes;
    std::vector<Primitive> primitives;
    std::vector<Texture> textures;
    std::vector<vk::Sampler> samplers;

    std::vector<size_t> root_nodes;
    size_t root_node_count{ 0 };

    VmaBuffer vertex_buffer;

private:
    uint32_t m_descriptor_count{ 0 };
    vk::DescriptorSet m_textures_descriptor_set;
    vk::DescriptorSetLayout m_textures_descriptor_set_layout;

    LightingInteractivity m_preferred_lighting;

    AssetManager &m_asset_manager;
    const glTFModel &m_model;
    Renderer &m_renderer;

    void add_sampler(const glTFModel::Sampler &sampler);
    void add_from_node(const glTFModel::Node &node);

    void upload_primitive_indices(Primitive &vk_primitive,
        const glTFModel::Primitive &primitive);
    void upload_model_vertices(const glTFModel &model);
};

}

#endif