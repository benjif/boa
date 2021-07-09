#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include "boa/macros.h"
#include "boa/gfx/scene.h"
#include "glm/gtc/type_ptr.hpp"
#include <stack>

namespace boa::gfx {

// assume primitives, nodes, and meshes only appear once
size_t Scene::add_nodes(tinygltf::Model &model, std::optional<size_t> parent, tinygltf::Node &node) {
    auto &new_node = m_nodes.emplace_back();
    size_t new_node_idx = m_nodes.size() - 1;
    new_node.index = new_node_idx;

    if (parent.has_value())
        new_node.parent = parent.value();

    LOG_INFO("Doing work for node {}", new_node_idx);

    if (node.mesh > -1) {
        auto &new_mesh = m_meshes.emplace_back();
        new_node.mesh = m_meshes.size() - 1;

        for (const auto &primitive : model.meshes[node.mesh].primitives) {
            auto &new_primitive = m_primitives.emplace_back();
            new_mesh.primitives.push_back(m_primitives.size() - 1);

            uint32_t vertex_start = static_cast<uint32_t>(m_vertices.size());

            if (primitive.indices < 0)
                TODO();

            const auto &index_accessor = model.accessors[primitive.indices];
            const auto &index_buffer_view = model.bufferViews[index_accessor.bufferView];
            const auto &index_buffer = model.buffers[index_buffer_view.buffer];

            const void *index_data_raw = &(index_buffer.data[index_accessor.byteOffset + index_buffer_view.byteOffset]);

            switch (index_accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t *index_data = static_cast<const uint32_t *>(index_data_raw);
                for (size_t i = 0; i < index_accessor.count; i++)
                    new_primitive.indices.push_back(index_data[i] + vertex_start);
                break;
            } case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t *index_data = static_cast<const uint16_t *>(index_data_raw);
                for (size_t i = 0; i < index_accessor.count; i++)
                    new_primitive.indices.push_back(index_data[i] + vertex_start);
                break;
            } case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t *index_data = static_cast<const uint8_t *>(index_data_raw);
                for (size_t i = 0; i < index_accessor.count; i++)
                    new_primitive.indices.push_back(index_data[i] + vertex_start);
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
                gltf::AttributeType attrib_type = gltf::attribute_types.at(attrib.first);
                const auto &accessor = model.accessors[primitive.attributes.at(attrib.first)];
                const auto &buffer_view = model.bufferViews[accessor.bufferView];
                const auto &buffer = model.buffers[buffer_view.buffer];

                const float *data
                    = reinterpret_cast<const float *>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

                /*if (primitive.material != -1) {
                    const auto &material = primitive.material
                }*/

                if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                    throw std::runtime_error("Component type isn't float");

                switch (attrib_type) {
                case gltf::AttributeType::Normal:
                    if (accessor.type != TINYGLTF_TYPE_VEC3)
                        throw std::runtime_error("Normal isn't vec3");
                    data_normal = data;
                    stride_normal = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                    break;
                case gltf::AttributeType::Position:
                    if (accessor.type != TINYGLTF_TYPE_VEC3)
                        throw std::runtime_error("Position isn't vec3");
                    position_accessor = accessor;
                    data_position = data;
                    stride_position = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                    break;
                case gltf::AttributeType::Tangent:
                    if (accessor.type != TINYGLTF_TYPE_VEC3)
                        throw std::runtime_error("Tangent isn't vec3");
                    data_tangent = data;
                    stride_tangent = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                    break;
                case gltf::AttributeType::Texcoord0:
                    if (accessor.type != TINYGLTF_TYPE_VEC2)
                        throw std::runtime_error("Texcoord0 isn't vec2");
                    data_texcoord0 = data;
                    stride_texcoord0 = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                    break;
                case gltf::AttributeType::Texcoord1:
                    if (accessor.type != TINYGLTF_TYPE_VEC2)
                        throw std::runtime_error("Texcoord1 isn't vec2");
                    data_texcoord1 = data;
                    stride_texcoord1 = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                    break;
                case gltf::AttributeType::Texcoord2:
                    if (accessor.type != TINYGLTF_TYPE_VEC2)
                        throw std::runtime_error("Texcoord2 isn't vec2");
                    stride_texcoord2 = accessor.ByteStride(buffer_view) ?
                        (accessor.ByteStride(buffer_view) / sizeof(float)) :
                        (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                    data_texcoord2 = data;
                    break;
                case gltf::AttributeType::Color0:
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

                m_vertices.push_back(std::move(vertex));
            }
        }
    }

    for (int child : node.children) {
        LOG_INFO("DOING CHILD {}", child);
        new_node.children.push_back(add_nodes(model, new_node_idx, model.nodes[child]));
    }

    return new_node_idx;
}

void Scene::add_from_gltf_file(const char *path) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    if (!ret)
        throw std::runtime_error("Failed to load gltf");

    if (model.scenes.empty()) {
        LOG_WARN("Attempted to add from glTF file without scenes");
        return;
    } else if (model.scenes.size() > 1) {
        LOG_WARN("Added from glTF file with more than one scene; only using default.");
    }

    const auto &scene = model.scenes[model.defaultScene];

    if (scene.nodes.size() == 0) {
        LOG_WARN("Attempted to add from glTF file without nodes");
        return;
    }

    add_nodes(model, std::nullopt, model.nodes[scene.nodes[0]]);
}

void Scene::debug_print_node(const Node &node) const {
    LOG_INFO("NODE #{}: ", node.index);

    if (node.mesh.has_value()) {
        LOG_INFO("  MESH #{}: ", node.mesh.value());
        const auto &mesh = m_meshes.at(node.mesh.value());

        for (const auto &primitive_idx : mesh.primitives) {
            LOG_INFO("      PRIMITIVE #{}: ", primitive_idx);
            const auto &primitive = m_primitives.at(primitive_idx);
            for (size_t index : primitive.indices) {
                const auto &pos = m_vertices.at(index).position;
                LOG_INFO("          VERTEX: ({}, {}, {})", pos.x, pos.y, pos.z);
            }
        }
    }

    for (size_t child_idx : node.children) {
        debug_print_node(m_nodes[child_idx]);
    }
}

void Scene::debug_print() const {
    LOG_INFO("Size of nodes: {}", m_nodes.size());
    debug_print_node(m_nodes[0]);
}

}