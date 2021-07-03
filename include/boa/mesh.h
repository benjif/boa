#ifndef BOA_MESH_H
#define BOA_MESH_H

#include "boa/types.h"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>

namespace boa {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texture_coord;
    glm::vec3 color;

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
                .format     = vk::Format::eR32G32Sfloat,
                .offset     = offsetof(Vertex, texture_coord),
            },
            vk::VertexInputAttributeDescription{
                .location   = 3,
                .binding    = 0,
                .format     = vk::Format::eR32G32B32Sfloat,
                .offset     = offsetof(Vertex, color),
            },
        };
    }

    bool operator==(const Vertex &other) const {
        return position == other.position && normal == other.normal && texture_coord == other.texture_coord;
    }
};

struct Mesh {
    void load_from_file(const char *path);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VmaBuffer vertex_buffer;
    VmaBuffer index_buffer;
};

}

namespace std {
    template<typename T> struct hash;
    template<> struct hash<boa::Vertex> {
        size_t operator()(const boa::Vertex &vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                    (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                    (hash<glm::vec2>()(vertex.texture_coord) << 1);
        }
    };
}

#endif