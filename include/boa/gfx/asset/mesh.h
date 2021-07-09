#ifndef BOA_GFX_MESH_H
#define BOA_GFX_MESH_H

#include "boa/gfx/vk/types.h"
#include "boa/gfx/scene.h"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>

/*namespace boa::gfx {

struct Mesh {
    void load_from_gltf_file(const char *path);
    void load_from_obj_file(const char *path);

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VmaBuffer vertex_buffer;
    VmaBuffer index_buffer;
};

}

namespace std {
    template<typename T> struct hash;
    template<> struct hash<boa::gfx::Vertex> {
        size_t operator()(const boa::gfx::Vertex &vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^
                    (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                    (hash<glm::vec2>()(vertex.texture_coord0) << 1);
        }
    };
}*/

#endif