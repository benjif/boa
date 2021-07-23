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
    vk::DescriptorSet texture_set{ VK_NULL_HANDLE };
    vk::Pipeline pipeline{ VK_NULL_HANDLE };
    vk::PipelineLayout pipeline_layout{ VK_NULL_HANDLE };
    glm::vec4 base_color;
    uint32_t descriptor_number;

    enum class ColorType {
        Vertex,
        Base,
        Texture,
    } color_type;
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
    Sphere bounding_sphere;
    uint32_t index_count;
    uint32_t material;
    VmaBuffer index_buffer;
};

struct Node {
    std::vector<uint32_t> primitives;
    std::vector<uint32_t> children;
    glm::mat4 transform_matrix;
    uint32_t id;
};

struct Model {
    Model(AssetManager &asset_manager, Renderer &renderer, const glTFModel &model, LightingInteractivity preferred_lighting);

    std::vector<Node> nodes;
    std::vector<Primitive> primitives;
    std::vector<uint32_t> root_nodes;

    Box bounding_box;

    VmaBuffer bounding_box_vertex_buffer;
    VmaBuffer vertex_buffer;

    LightingInteractivity lighting;

private:
    vk::DescriptorSet m_textures_descriptor_set;
    vk::DescriptorSetLayout m_textures_descriptor_set_layout;

    uint32_t m_descriptor_count{ 0 };

    vk::Sampler create_sampler(Renderer &renderer, const glTFModel::Sampler &sampler);
    void add_from_node(AssetManager &asset_manager, Renderer &renderer, const glTFModel &model, const glTFModel::Node &node);
    void calculate_model_bounding_box(const glTFModel &model, const glTFModel::Node &node, glm::mat4 transform_matrix);

    void upload_primitive_indices(Renderer &renderer, Primitive &vk_primitive,
        const glTFModel::Primitive &primitive);
    void upload_model_vertices(Renderer &renderer, const glTFModel &model);
    void upload_bounding_box_vertices(Renderer &renderer);
};

}

#endif
