#ifndef BOA_GFX_SCENE_H
#define BOA_GFX_SCENE_H

#include "boa/macros.h"
#include "boa/gfx/asset/gltf.h"
#include "glm/glm.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <optional>

namespace tinygltf {
class Model;
class Node;
}

namespace boa::gfx {

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

    static std::array<vk::VertexInputAttributeDescription, 4> get_attribute_descriptions() {
        return std::array<vk::VertexInputAttributeDescription, 4>{
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
                //.format     = vk::Format::eR32G32B32Sfloat,
                .offset     = offsetof(Vertex, color0),
            },
            vk::VertexInputAttributeDescription{
                .location   = 3,
                .binding    = 0,
                .format     = vk::Format::eR32G32Sfloat,
                .offset     = offsetof(Vertex, texture_coord0),
            },
        };
    }

    bool operator==(const Vertex &other) const {
        return position == other.position && normal == other.normal && texture_coord0 == other.texture_coord0;
    }
};

class Scene {
    REMOVE_COPY_AND_ASSIGN(Scene);
public:
    Scene() {}

    void add_from_gltf_file(const char *path);
    void debug_print() const;

private:
    struct Primitive {
        size_t material;
        std::vector<size_t> indices;
    };

    struct Texture {
    };

    struct Mesh {
        size_t material;
        glm::vec3 min, max;
        std::vector<size_t> primitives;
    };

    struct Node {
        size_t index;
        std::optional<size_t> parent;
        std::optional<size_t> mesh;
        std::vector<size_t> children;
    };

    void debug_print_node(const Node &node) const;

    size_t add_nodes(tinygltf::Model &model, std::optional<size_t> parent, tinygltf::Node &node);

    std::vector<Node> m_nodes;
    std::vector<Mesh> m_meshes;
    std::vector<Primitive> m_primitives;

    std::vector<Vertex> m_vertices;
};

}

#endif