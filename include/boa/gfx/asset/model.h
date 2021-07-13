#ifndef BOA_GFX_ASSET_SCENE_H
#define BOA_GFX_ASSET_SCENE_H

#include "boa/macros.h"
#include "boa/iteration.h"
#include "boa/gfx/asset/gltf.h"
#include "boa/gfx/asset/linear_types.h"
#include "tiny_gltf.h"
#include <vector>
#include <optional>
#include <functional>

namespace boa::gfx {

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
        Box bounding_box;
        Sphere bounding_sphere;
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
        //glm::vec3 min, max; // TODO
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
    size_t get_image_count() const;

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
    void for_each_image(std::function<Iteration(const Image &)> callback) const;

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