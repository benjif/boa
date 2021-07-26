#ifndef BOA_GFX_ASSET_SCENE_H
#define BOA_GFX_ASSET_SCENE_H

#include "boa/utl/macros.h"
#include "boa/utl/iteration.h"
#include "boa/gfx/linear.h"
#include "glm/gtc/quaternion.hpp"
#include "tiny_gltf.h"
#include <vector>
#include <optional>
#include <functional>

namespace boa::gfx {

enum class AttributeType {
    Normal,
    Position,
    Tangent,
    Texcoord0,
    Texcoord1,
    Texcoord2,
    Color0,
    Joint0,
    Weights0,
};

static const std::unordered_map<std::string, AttributeType> attribute_types = {
    { "NORMAL",     AttributeType::Normal       },
    { "POSITION",   AttributeType::Position     },
    { "TANGENT",    AttributeType::Tangent      },
    { "TEXCOORD_0", AttributeType::Texcoord0    },
    { "TEXCOORD_1", AttributeType::Texcoord1    },
    { "TEXCOORD_2", AttributeType::Texcoord2    },
    { "COLOR_0",    AttributeType::Color0       },
    { "JOINT_0",    AttributeType::Joint0       },
    { "WEIGHTS_0",  AttributeType::Weights0     },
};

class glTFModel {
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
        Box bounding_box;
        Sphere bounding_sphere;
        std::optional<size_t> material;
        std::vector<uint32_t> indices;
        bool has_vertex_coloring{ false };
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

        struct MetallicRoughness {
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
        std::vector<size_t> primitives;
    };

    struct Node {
        size_t id;
        std::vector<size_t> children;
        std::optional<size_t> parent;
        std::optional<size_t> mesh;
        glm::dquat rotation;
        glm::dvec3 translation;
        glm::dvec3 scale;
        glm::dmat4 matrix;
    };

    struct AnimationChannel {
        enum class Path {
            Translation,
            Rotation,
            Scale,
            Weights,
        };

        struct Target {
            size_t node;
            Path path;
        } target;
        size_t sampler;
    };

    struct AnimationSampler {
        enum class Interpolation {
            Linear,
            Step,
            CubicSpline,
        };

        std::vector<float> in, out;
        Interpolation interpolation;
    };

    struct Animation {
        std::vector<AnimationChannel> channels;
        std::vector<AnimationSampler> samplers;
    };

    glTFModel() {}
    glTFModel(const char *path) { open_gltf_file(path); }
    void open_gltf_file(const char *path);
    void debug_print() const;

    std::string get_file_path() const { return m_path; }

    const std::vector<Vertex> &get_vertices() const;
    const std::vector<size_t> &get_root_nodes() const;
    size_t get_root_node_count() const;
    size_t get_node_count() const;
    size_t get_primitive_count() const;
    size_t get_texture_count() const;
    size_t get_sampler_count() const;
    size_t get_image_count() const;
    size_t get_animation_count() const;

    const Node &get_node(size_t index) const;
    const Primitive &get_primitive(size_t index) const;
    const Mesh &get_mesh(size_t index) const;
    const Material &get_material(size_t index) const;
    const Texture &get_texture(size_t index) const;
    const Image &get_image(size_t index) const;
    const Sampler &get_sampler(size_t index) const;
    const Animation &get_animation(size_t index) const;

    template <typename C>
    void for_each_node(C callback) const {
        for (const auto &node : m_nodes) {
            auto decision = callback(node);
            if (decision == Iteration::Break)
                break;
            else if (decision == Iteration::Continue)
                continue;
        }
    }

    template <typename C>
    void for_each_root_node(C callback) const {
        for (size_t node_idx : m_root_nodes) {
            const auto &node = m_nodes[node_idx];
            auto decision = callback(node);
            if (decision == Iteration::Break)
                break;
            else if (decision == Iteration::Continue)
                continue;
        }
    }

    template <typename C>
    void for_each_primitive(C callback) const {
        for (const auto &primitive : m_primitives) {
            auto decision = callback(primitive);
            if (decision == Iteration::Break)
                break;
            else if (decision == Iteration::Continue)
                continue;
        }
    }

    template <typename C>
    void for_each_material(C callback) const {
        for (const auto &material : m_materials) {
            auto decision = callback(material);
            if (decision == Iteration::Break)
                break;
            else if (decision == Iteration::Continue)
                continue;
        }
    }

    template <typename C>
    void for_each_sampler(C callback) const {
        for (const auto &sampler : m_samplers) {
            auto decision = callback(sampler);
            if (decision == Iteration::Break)
                break;
            else if (decision == Iteration::Continue)
                continue;
        }
    }

    template <typename C>
    void for_each_image(C callback) const {
        for (const auto &image : m_images) {
            auto decision = callback(image);
            if (decision == Iteration::Break)
                break;
            else if (decision == Iteration::Continue)
                continue;
        }
    }

    template <typename C>
    void for_each_animation(C callback) const {
        for (const auto &animation : m_animations) {
            auto decision = callback(animation);
            if (decision == Iteration::Break)
                break;
            else if (decision == Iteration::Continue)
                continue;
        }
    }

private:
    bool m_initialized{ false };
    std::string m_path;

    void debug_print_node(const Node &node, uint32_t indent) const;

    std::vector<Vertex> m_vertices;

    std::vector<Node> m_nodes;
    std::vector<Mesh> m_meshes;
    std::vector<Primitive> m_primitives;
    std::vector<Texture> m_textures; 
    std::vector<Image> m_images;
    std::vector<Material> m_materials;
    std::vector<Sampler> m_samplers;
    std::vector<Animation> m_animations;

    std::vector<size_t> m_root_nodes;

    tinygltf::TinyGLTF m_loader;
    tinygltf::Model m_model;
};

}

#endif
