#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "boa/macros.h"
#include "boa/gfx/asset/model.h"
#include "glm/gtc/type_ptr.hpp"
#include <stack>

namespace boa::gfx {

// assume primitives, nodes, and meshes only appear once
size_t Model::add_nodes(std::optional<size_t> parent, tinygltf::Node &node) {
    Node new_node;
    if (node.matrix.size() == 16)
        new_node.matrix = glm::make_mat4<double>(&node.matrix.data()[0]);
    else
        new_node.matrix = glm::mat4(1.0f);
    size_t new_node_idx = m_nodes.size();

    if (parent.has_value())
        new_node.parent = parent.value();

    if (node.mesh > -1) {
        Mesh new_mesh;
        new_node.mesh = m_meshes.size();

        for (const auto &primitive : m_model.meshes[node.mesh].primitives) {
            Primitive new_primitive;
            new_mesh.primitives.push_back(m_primitives.size());

            uint32_t vertex_start = static_cast<uint32_t>(m_vertices.size());
            new_primitive.vertex_offset = vertex_start * sizeof(Vertex);

            if (primitive.indices < 0)
                TODO();
            if (primitive.material != -1)
                new_primitive.material = primitive.material;

            const auto &index_accessor = m_model.accessors[primitive.indices];
            const auto &index_buffer_view = m_model.bufferViews[index_accessor.bufferView];
            const auto &index_buffer = m_model.buffers[index_buffer_view.buffer];

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
                const auto &accessor = m_model.accessors[primitive.attributes.at(attrib.first)];
                const auto &buffer_view = m_model.bufferViews[accessor.bufferView];
                const auto &buffer = m_model.buffers[buffer_view.buffer];

                const float *data
                    = reinterpret_cast<const float *>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

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
            if (position_accessor.minValues.size() != 3 || position_accessor.maxValues.size() != 3)
                throw std::runtime_error("Position accessor doesn't contain min or max values");

            new_primitive.bounding_box.min = glm::make_vec3<double>(position_accessor.minValues.data());
            new_primitive.bounding_box.max = glm::make_vec3<double>(position_accessor.maxValues.data());
            m_primitives.push_back(std::move(new_primitive));

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
                    vertex.color0 = glm::vec4(1.0f);
                }

                m_vertices.push_back(std::move(vertex));
            }
        }

        m_meshes.push_back(std::move(new_mesh));
    }

    m_nodes.push_back(std::move(new_node));

    for (int child : node.children) {
        size_t child_idx = add_nodes(new_node_idx, m_model.nodes[child]);
        m_nodes[new_node_idx].children.push_back(child_idx);
    }

    return new_node_idx;
}

void Model::open_gltf_file(const char *path) {
    LOG_INFO("(glTF) Opening file '{}'", path);

    if (m_initialized)
        return;
    std::string err, warn;

    bool ret = m_loader.LoadASCIIFromFile(&m_model, &err, &warn, path);
    if (!ret)
        throw std::runtime_error("Failed to load gltf");

    if (m_model.scenes.empty()) {
        LOG_WARN("(glTF) Attempted to add from glTF file without a scene");
        return;
    } else if (m_model.scenes.size() > 1) {
        LOG_WARN("(glTF) Added from glTF file with more than one scene; only using default.");
    }

    const auto &scene = m_model.scenes[m_model.defaultScene];

    if (scene.nodes.size() == 0) {
        LOG_WARN("(glTF) Attempted to add from glTF file without nodes");
        return;
    }

    m_nodes.reserve(scene.nodes.size());
    m_meshes.reserve(m_model.meshes.size());
    m_materials.reserve(m_model.materials.size());
    m_textures.reserve(m_model.textures.size());
    m_samplers.reserve(m_model.samplers.size());
    m_images.reserve(m_model.images.size());

    for (const auto &material : m_model.materials) {
        Material new_material;
        new_material.metallic_roughness = Material::MetallicRoughness{
            .metallic_factor    = material.pbrMetallicRoughness.metallicFactor,
            .roughness_factor   = material.pbrMetallicRoughness.roughnessFactor,
        };
        if (material.pbrMetallicRoughness.baseColorTexture.index >= 0)
            new_material.metallic_roughness.base_color_texture = material.pbrMetallicRoughness.baseColorTexture.index;
        if (material.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
            new_material.metallic_roughness.metallic_roughness_texture = material.pbrMetallicRoughness.metallicRoughnessTexture.index;
        if (material.pbrMetallicRoughness.baseColorFactor.size() == 4)
            std::copy_n(material.pbrMetallicRoughness.baseColorFactor.begin(), 4, new_material.metallic_roughness.base_color_factor.begin());
        if (material.normalTexture.index >= 0)
            new_material.normal_texture = material.normalTexture.index;
        if (material.occlusionTexture.index >= 0)
            new_material.occlusion_texture = material.occlusionTexture.index;
        if (material.emissiveTexture.index >= 0)
            new_material.emissive_texture = material.emissiveTexture.index;
        if (material.emissiveFactor.size() == 3)
            std::copy_n(material.emissiveFactor.begin(), 3, new_material.emissive_factor.begin());
        if (material.alphaMode.compare("MASK") == 0)
            new_material.alpha_mode = Material::AlphaMode::Mask;
        else if (material.alphaMode.compare("BLEND") == 0)
            new_material.alpha_mode = Material::AlphaMode::Blend;
        new_material.alpha_cutoff = material.alphaCutoff;
        new_material.double_sided = material.doubleSided;

        m_materials.push_back(std::move(new_material));
    }

    for (const auto &sampler : m_model.samplers) {
        m_samplers.push_back({
            sampler.magFilter,
            sampler.minFilter,
            sampler.wrapS,
            sampler.wrapT,
        });
    }

    for (const auto &texture : m_model.textures) {
        Texture new_texture;
        if (texture.sampler >= 0)
            new_texture.sampler = texture.sampler;
        if (texture.source >= 0)
            new_texture.source = texture.source;
        m_textures.push_back(std::move(new_texture));
    }

    for (const auto &image : m_model.images) {
        Image new_image;
        new_image.bit_depth = image.bits;
        new_image.component = image.component;
        new_image.width = image.width;
        new_image.height = image.height;
        new_image.data = (void *)image.image.data();
        m_images.push_back(std::move(new_image));
    }

    add_nodes(std::nullopt, m_model.nodes[scene.nodes[0]]);
}

void Model::debug_print_node(const Node &node) const {
    LOG_INFO("(glTF) NODE: ");
    if (node.mesh.has_value()) {
        LOG_INFO("  MESH #{}: ", node.mesh.value());
        const auto &mesh = m_meshes.at(node.mesh.value());

        for (const auto &primitive_idx : mesh.primitives) {
            LOG_INFO("      PRIMITIVE #{}: ", primitive_idx);
            const auto &primitive = m_primitives.at(primitive_idx);
            if (primitive.material.has_value())
                LOG_INFO("      HAS MATERIAL: #{}", primitive.material.value());
            for (size_t index : primitive.indices) {
                const auto &vert = m_vertices.at(index);
                LOG_INFO("          VERTEX @ INDEX {}: {}", index, vert);
            }
        }
    }

    for (size_t child_idx : node.children) {
        debug_print_node(m_nodes[child_idx]);
    }
}

void Model::debug_print() const {
    LOG_INFO("(glTF) Size of nodes: {}", m_nodes.size());
    debug_print_node(m_nodes[0]);
}

const std::vector<Vertex> &Model::get_vertices() const {
    return m_vertices;
}

size_t Model::get_primitive_count() const {
    return m_primitives.size();
}

size_t Model::get_texture_count() const {
    return m_textures.size();
}

size_t Model::get_sampler_count() const {
    return m_samplers.size();
}

size_t Model::get_image_count() const {
    return m_images.size();
}

const Model::Node &Model::get_node(size_t index) const {
    return m_nodes.at(index);
}

const Model::Primitive &Model::get_primitive(size_t index) const {
    return m_primitives.at(index);
}

const Model::Mesh &Model::get_mesh(size_t index) const {
    return m_meshes.at(index);
}

const Model::Material &Model::get_material(size_t index) const {
    return m_materials.at(index);
}

const Model::Texture &Model::get_texture(size_t index) const {
    return m_textures.at(index);
}

const Model::Image &Model::get_image(size_t index) const {
    return m_images.at(index);
}

const Model::Sampler &Model::get_sampler(size_t index) const {
    return m_samplers.at(index);
}

void Model::for_each_node(std::function<Iteration(const Node &)> callback) const {
    for (const auto &node : m_nodes) {
        auto decision = callback(node);
        if (decision == Iteration::Break)
            break;
        else if (decision == Iteration::Continue)
            continue;
    }
}

void Model::for_each_primitive(std::function<Iteration(const Primitive &)> callback) const {
    for (const auto &primitive : m_primitives) {
        auto decision = callback(primitive);
        if (decision == Iteration::Break)
            break;
        else if (decision == Iteration::Continue)
            continue;
    }
}

void Model::for_each_material(std::function<Iteration(const Material &)> callback) const {
    for (const auto &material : m_materials) {
        auto decision = callback(material);
        if (decision == Iteration::Break)
            break;
        else if (decision == Iteration::Continue)
            continue;
    }
}

void Model::for_each_sampler(std::function<Iteration(const Sampler &)> callback) const {
    for (const auto &sampler : m_samplers) {
        auto decision = callback(sampler);
        if (decision == Iteration::Break)
            break;
        else if (decision == Iteration::Continue)
            continue;
    }
}

void Model::for_each_image(std::function<Iteration(const Image &)> callback) const {
    for (const auto &image : m_images) {
        auto decision = callback(image);
        if (decision == Iteration::Break)
            break;
        else if (decision == Iteration::Continue)
            continue;
    }
}

}