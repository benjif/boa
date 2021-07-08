#define TINYGLTF_IMPLEMENTATION
//#define TINYGTLF_NO_EXTERNAL_IMAGE
#define TINYGTLF_NO_INCLUDE_STB_IMAGE
#define TINYGTLF_NO_INCLUDE_STB_IMAGE_WRITE
#include "tiny_obj_loader.h"
#include "tiny_gltf.h"
#include "boa/gfx/asset/obj/mesh.h"
#include "glm/gtc/type_ptr.hpp"
#include <stack>
#include <vector>

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

void Mesh::load_from_gltf_file(const char *path) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!ret)
        throw std::runtime_error("Failed to load gltf");

    if (model.scenes.empty())
        return;

    const auto &scene = model.scenes[model.defaultScene];

    if (scene.nodes.size() == 0)
        return;

    std::stack<int, std::vector<int>> nodes;
    nodes.push(scene.nodes[0]);

    while (!nodes.empty()) {
        int node_index = nodes.top();
        nodes.pop();

        const auto &node = model.nodes[node_index];

        for (int child : node.children)
            nodes.push(child);

        if (node.mesh < 0)
            continue;

        const auto &mesh = model.meshes[node.mesh];
        for (const auto &primitive : mesh.primitives) {
            uint32_t vertex_start = static_cast<uint32_t>(vertices.size());

            if (primitive.indices < 0)
                throw std::runtime_error("Primitive doesn't have indices");

            const auto &index_accessor = model.accessors[primitive.indices];
            const auto &index_buffer_view = model.bufferViews[index_accessor.bufferView];
            const auto &index_buffer = model.buffers[index_buffer_view.buffer];

            const void *index_data_raw = &(index_buffer.data[index_accessor.byteOffset + index_buffer_view.byteOffset]);

            switch (index_accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t *index_data = static_cast<const uint32_t *>(index_data_raw);
                for (size_t i = 0; i < index_accessor.count; i++)
                    indices.push_back(index_data[i] + vertex_start);
                break;
            } case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t *index_data = static_cast<const uint16_t *>(index_data_raw);
                for (size_t i = 0; i < index_accessor.count; i++)
                    indices.push_back(index_data[i] + vertex_start);
                break;
            } case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t *index_data = static_cast<const uint8_t *>(index_data_raw);
                for (size_t i = 0; i < index_accessor.count; i++)
                    indices.push_back(index_data[i] + vertex_start);
                break;
            } default:
                throw std::runtime_error("Index component type not supported");
                break;
            }

            const float *data_normal = nullptr,
                        *data_position = nullptr,
                        *data_tangent = nullptr,
                        *data_texcoord0 = nullptr,
                        *data_texcoord1 = nullptr,
                        *data_texcoord2 = nullptr,
                        *data_color0 = nullptr;

            uint32_t stride_normal,
                     stride_position,
                     stride_tangent,
                     stride_texcoord0,
                     stride_texcoord1,
                     stride_texcoord2,
                     stride_color0;

            int type_color0;

            tinygltf::Accessor position_accessor;

            for (const auto &attrib : primitive.attributes) {
                AttributeType attrib_type = attribute_types.at(attrib.first);
                const auto &accessor = model.accessors[primitive.attributes.at(attrib.first)];
                const auto &buffer_view = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[buffer_view.buffer];

                const float *data
                    = reinterpret_cast<const float *>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

                /*if (primitive.material != -1) {
                    const auto &material = primitive.material
                }*/

                switch (attrib_type) {
                case AttributeType::Normal:
                    data_normal = data;
                    stride_normal = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                    break;
                case AttributeType::Position:
                    position_accessor = accessor;
                    data_position = data;
                    stride_position = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                    break;
                case AttributeType::Tangent:
                    data_tangent = data;
                    stride_tangent = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                    break;
                case AttributeType::Texcoord0:
                    data_texcoord0 = data;
                    stride_texcoord0 = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                    break;
                case AttributeType::Texcoord1:
                    data_texcoord1 = data;
                    stride_texcoord1 = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                    break;
                case AttributeType::Texcoord2:
                    stride_texcoord2 = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                    data_texcoord2 = data;
                    break;
                case AttributeType::Color0:
                    stride_color0 = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type));
                    data_color0 = data;
                    type_color0 = accessor.type;
                    break;
                }
            }

            if (!data_position)
                throw std::runtime_error("glTF primitive must have a position attribute");

            for (size_t i = 0; i < position_accessor.count; i++) {
                Vertex vertex{};
                vertex.position = glm::make_vec3(&data_position[i * stride_position]);
                vertex.normal = data_normal ? glm::normalize(glm::make_vec3(&data_normal[i * stride_normal])) : glm::vec3(0.0f);
                vertex.texture_coord0 = data_texcoord0 ? glm::make_vec2(&data_texcoord0[i * stride_texcoord0]) : glm::vec2(0.0f);
                vertex.texture_coord1 = data_texcoord1 ? glm::make_vec2(&data_texcoord1[i * stride_texcoord1]) : glm::vec2(0.0f);

                if (data_color0) {
                    if (type_color0 == TINYGLTF_TYPE_VEC3) {
                        vertex.color0 = glm::vec4{
                            data_color0[i * stride_color0 + 0 * sizeof(float)],
                            data_color0[i * stride_color0 + 1 * sizeof(float)],
                            data_color0[i * stride_color0 + 2 * sizeof(float)],
                            1.0f,
                        };
                    } else if (type_color0 == TINYGLTF_TYPE_VEC4) {
                        vertex.color0 = glm::make_vec4(&data_color0[i * stride_color0]);
                    } else {
                        throw std::runtime_error("Unknown glTF color type");
                    }
                } else {
                    vertex.color0 = glm::vec4(0.0f);
                }

                vertices.push_back(std::move(vertex));
            }
        }
    }
}

void Mesh::load_from_obj_file(const char *path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
        throw std::runtime_error(warn + err);

    std::unordered_map<Vertex, uint32_t> unique_vertices;

    unique_vertices.reserve(attrib.vertices.size() * 0.25);
    vertices.reserve(attrib.vertices.size());
    indices.reserve(shapes.size());

    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2],
            };

            vertex.texture_coord0 = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1],
            };

            //vertex.color = { 1.0f, 1.0f, 1.0f };
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2],
            };

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(unique_vertices[vertex]);
        }
    }
}

}