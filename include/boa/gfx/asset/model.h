#ifndef BOA_GFX_ASSET_SCENE_H
#define BOA_GFX_ASSET_SCENE_H

#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/gltf.h"
#include "glm/glm.hpp"
#include "tiny_gltf.h"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>
#include <functional>

namespace boa::gfx {

// TODO: move elsewhere
struct Vertex {
    glm::vec3 position, normal;
    glm::vec4 color0;
    glm::vec2 texture_coord0, texture_coord1;

    // others?
    // tangent, joint0, weights0

    static vk::VertexInputBindingDescription get_binding_description() {
        return {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = vk::VertexInputRate::eVertex,
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 5> get_attribute_descriptions() {
        return std::array<vk::VertexInputAttributeDescription, 5>{
            vk::VertexInputAttributeDescription{
                .location   = 0,
                .binding    = 0,
                .format     = vk::Format::eR32G32B32Sfloat,
                .offset     = offsetof(Vertex, position),
            },
            vk::VertexInputAttributeDescription{
                .location   = 1,
                .binding    = 0,
                .format     = vk::Format::eR32G32B32Sfloat,
                .offset     = offsetof(Vertex, normal),
            },
            vk::VertexInputAttributeDescription{
                .location   = 2,
                .binding    = 0,
                .format     = vk::Format::eR32G32B32A32Sfloat,
                .offset     = offsetof(Vertex, color0),
            },
            vk::VertexInputAttributeDescription{
                .location   = 3,
                .binding    = 0,
                .format     = vk::Format::eR32G32Sfloat,
                .offset     = offsetof(Vertex, texture_coord0),
            },
            vk::VertexInputAttributeDescription{
                .location   = 4,
                .binding    = 0,
                .format     = vk::Format::eR32G32Sfloat,
                .offset     = offsetof(Vertex, texture_coord1),
            },
        };
    }

    bool operator==(const Vertex &other) const {
        return position == other.position && normal == other.normal && texture_coord0 == other.texture_coord0;
    }
};

class Model {
    REMOVE_COPY_AND_ASSIGN(Model);
public:
    struct Sampler {
        int mag_filter, min_filter;
        int wrap_s_mode, wrap_t_mode;
    };

    struct Image {
        uint32_t width, height;
        int bit_depth;
        int component;
        void *data;
    };

    struct Primitive {
        size_t vertex_offset;
        std::optional<size_t> material;
        std::vector<uint32_t> indices;
    };

    struct Texture {
        std::optional<size_t> sampler;
        std::optional<size_t> source;
    };

    struct Material {
        enum class AlphaMode {
            Opaque,
            Mask,
            Blend,
        };

        struct {
            std::array<double, 4> base_color_factor{ 1.0f, 1.0f, 1.0f, 1.0f };
            double metallic_factor{ 1.0f };
            double roughness_factor{ 1.0f };
            std::optional<size_t> base_color_texture;
            std::optional<size_t> metallic_roughness_texture;
        } metallic_roughness;

        std::optional<size_t> normal_texture;
        std::optional<size_t> occlusion_texture;
        std::optional<size_t> emissive_texture;
        std::array<double, 3> emissive_factor{ 0.0f, 0.0f, 0.0f };
        AlphaMode alpha_mode{ AlphaMode::Opaque };
        double alpha_cutoff{ 0.5f };
        bool double_sided{ false };
    };

    struct Mesh {
        glm::vec3 min, max; // TODO
        std::vector<size_t> primitives;
    };

    struct Node {
        glm::dmat4 matrix;
        std::optional<size_t> parent;
        std::optional<size_t> mesh;
        std::vector<size_t> children;
    };

    Model() {}
    void open_gltf_file(const char *path);
    void debug_print() const;

    const std::vector<Vertex> &get_vertices() const;
    size_t get_primitive_count() const;
    size_t get_texture_count() const;
    size_t get_sampler_count() const;

    const Node &get_node(size_t index) const;
    const Primitive &get_primitive(size_t index) const;
    const Mesh &get_mesh(size_t index) const;
    const Material &get_material(size_t index) const;
    const Texture &get_texture(size_t index) const;
    const Image &get_image(size_t index) const;
    const Sampler &get_sampler(size_t index) const;

    void for_each_node(std::function<Iteration(const Node &)> callback) const;
    void for_each_primitive(std::function<Iteration(const Primitive &)> callback) const;
    void for_each_material(std::function<Iteration(const Material &)> callback) const;
    void for_each_sampler(std::function<Iteration(const Sampler &)> callback) const;

private:
    bool m_initialized{ false };

    void debug_print_node(const Node &node) const;
    size_t add_nodes(std::optional<size_t> parent, tinygltf::Node &node);

    std::vector<Vertex> m_vertices;

    std::vector<Node> m_nodes;
    std::vector<Mesh> m_meshes;
    std::vector<Primitive> m_primitives;
    std::vector<Texture> m_textures; 
    std::vector<Image> m_images;
    std::vector<Material> m_materials;
    std::vector<Sampler> m_samplers;

    tinygltf::TinyGLTF m_loader;
    tinygltf::Model m_model;
};

}

#endif