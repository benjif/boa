#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_USE_CPP14
#define TINYGLTF_USE_RAPIDJSON
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "boa/utl/macros.h"
#include "boa/gfx/asset/gltf_model.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtx/transform.hpp"
#include <stack>

namespace boa::gfx {

void glTFModel::open_gltf_file(const char *path) {
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
    m_animations.reserve(m_model.animations.size());

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
        m_images.push_back(Image{
            .width      = (uint32_t)image.width,
            .height     = (uint32_t)image.height,
            .bit_depth  = image.bits,
            .component  = image.component,
            .data       = (void *)image.image.data(),
        });
    }

    for (const auto &animation : m_model.animations) {
        Animation new_animation;
        new_animation.channels.reserve(animation.channels.size());
        new_animation.samplers.reserve(animation.samplers.size());

        for (const auto &channel : animation.channels) {
            AnimationChannel new_channel{
                .target     = {
                    .node   = (size_t)channel.target_node,
                },
                .sampler    = (size_t)channel.sampler,
            };

            if (channel.target_path.compare("translation") == 0)
                new_channel.target.path = AnimationChannel::Path::Translation;
            else if (channel.target_path.compare("rotation") == 0)
                new_channel.target.path = AnimationChannel::Path::Rotation;
            else if (channel.target_path.compare("scale") == 0)
                new_channel.target.path = AnimationChannel::Path::Scale;
            else if (channel.target_path.compare("weights") == 0)
                new_channel.target.path = AnimationChannel::Path::Weights;
            else
                throw std::runtime_error("Unknown animation channel path");

            const auto &sampler = animation.samplers[channel.sampler];

            const auto &in_accessor = m_model.accessors[sampler.input];
            const auto &in_buffer_view = m_model.bufferViews[in_accessor.bufferView];
            const auto &in_buffer = m_model.buffers[in_buffer_view.buffer];
            const float *in_data_raw = reinterpret_cast<const float *>(&(in_buffer.data[in_accessor.byteOffset + in_buffer_view.byteOffset]));

            const auto &out_accessor = m_model.accessors[sampler.output];
            const auto &out_buffer_view = m_model.bufferViews[out_accessor.bufferView];
            const auto &out_buffer = m_model.buffers[out_buffer_view.buffer];
            const float *out_data_raw = reinterpret_cast<const float *>(&(out_buffer.data[out_accessor.byteOffset + out_buffer_view.byteOffset]));

            uint32_t in_stride = in_accessor.ByteStride(in_buffer_view) ?
                (in_accessor.ByteStride(in_buffer_view) / sizeof(float)) : 1;
            uint32_t out_stride = 0, out_floats = 0;
            switch (new_channel.target.path) {
            case AnimationChannel::Path::Translation:
                out_stride = out_accessor.ByteStride(out_buffer_view) ?
                    (out_accessor.ByteStride(out_buffer_view) / sizeof(float)) :
                    (tinygltf::GetComponentSizeInBytes(out_accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                out_floats = 3;
                break;
            case AnimationChannel::Path::Rotation:
                out_stride = out_accessor.ByteStride(out_buffer_view) ?
                    (out_accessor.ByteStride(out_buffer_view) / sizeof(float)) :
                    (tinygltf::GetComponentSizeInBytes(out_accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4));
                out_floats = 4;
                break;
            case AnimationChannel::Path::Scale:
                out_stride = out_accessor.ByteStride(out_buffer_view) ?
                    (out_accessor.ByteStride(out_buffer_view) / sizeof(float)) :
                    (tinygltf::GetComponentSizeInBytes(out_accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                out_floats = 3;
                break;
            case AnimationChannel::Path::Weights:
                out_stride = out_accessor.ByteStride(out_buffer_view) ?
                    (out_accessor.ByteStride(out_buffer_view) / sizeof(float)) :
                    (tinygltf::GetComponentSizeInBytes(out_accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_SCALAR));
                out_floats = 1;
                break;
            }

            if (in_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || out_accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                throw std::runtime_error("Unsupported input or output component type for animation data");
            if (in_accessor.count != out_accessor.count)
                throw std::runtime_error("Animation sampler doesn't have equal number of input and output values");

            AnimationSampler new_sampler;

            if (sampler.interpolation.compare("LINEAR") == 0)
                new_sampler.interpolation = AnimationSampler::Interpolation::Linear;
            else if (sampler.interpolation.compare("STEP") == 0)
                new_sampler.interpolation = AnimationSampler::Interpolation::Step;
            else if (sampler.interpolation.compare("CUBICSPLINE") == 0)
                new_sampler.interpolation = AnimationSampler::Interpolation::CubicSpline;
            else
                throw std::runtime_error("Animation sampler interpolation is unknown");

            new_sampler.in.reserve(in_accessor.count);
            new_sampler.out.reserve(out_accessor.count * out_floats);

            for (size_t i = 0; i < in_accessor.count; i++) {
                new_sampler.in.push_back(in_data_raw[i * in_stride]);
                for (size_t j = 0; j < out_floats; j++)
                    new_sampler.out.push_back(out_data_raw[i * out_stride + j]);
            }

            new_animation.channels.push_back(std::move(new_channel));
            new_animation.samplers.push_back(std::move(new_sampler));
        }

        m_animations.push_back(std::move(new_animation));
    }

    std::copy(scene.nodes.begin(), scene.nodes.end(), std::back_inserter(m_root_nodes));

    for (const auto &node : m_model.nodes) {
        Node new_node;
        if (node.translation.size() == 3)
            new_node.translation = glm::make_vec3<double>(&node.translation.data()[0]);
        else
            new_node.translation = glm::dvec3(0.0f, 0.0f, 0.0f);

        if (node.rotation.size() == 4)
            new_node.rotation = glm::make_quat<double>(&node.rotation.data()[0]);
        else
            new_node.rotation = glm::dquat(0.0f, 0.0f, 0.0f, 0.0f);

        if (node.scale.size() == 3)
            new_node.scale = glm::make_vec3<double>(&node.scale.data()[0]);
        else
            new_node.scale = glm::dvec3(1.0f, 1.0f, 1.0f);

        if (node.matrix.size() == 16)
            new_node.matrix = glm::make_mat4<double>(&node.matrix.data()[0]);
        else
            new_node.matrix = glm::translate(new_node.translation) * glm::toMat4(new_node.rotation) * glm::scale(new_node.scale);

        size_t new_node_idx = m_nodes.size();
        new_node.id = new_node_idx;

        if (node.mesh > -1) {
            Mesh new_mesh;
            new_node.mesh = m_meshes.size();

            for (const auto &primitive : m_model.meshes[node.mesh].primitives) {
                Primitive new_primitive;
                new_mesh.primitives.push_back(m_primitives.size());

                uint32_t vertex_start = static_cast<uint32_t>(m_vertices.size());

                if (primitive.indices < 0)
                    LOG_FAIL("Primitive doesn't have list of indices (unsupported)");
                if (primitive.material != -1)
                    new_primitive.material = primitive.material;
                if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                    LOG_FAIL("Unsupported primitive mode (not a triangle list)");

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
                    AttributeType attrib_type = attribute_types.at(attrib.first);
                    const auto &accessor = m_model.accessors[primitive.attributes.at(attrib.first)];
                    const auto &buffer_view = m_model.bufferViews[accessor.bufferView];
                    const auto &buffer = m_model.buffers[buffer_view.buffer];

                    const float *data
                        = reinterpret_cast<const float *>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);

                    if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
                        throw std::runtime_error("Component type isn't float");

                    switch (attrib_type) {
                    case AttributeType::Normal:
                        if (accessor.type != TINYGLTF_TYPE_VEC3)
                            throw std::runtime_error("Normal isn't vec3");
                        data_normal = data;
                        stride_normal = accessor.ByteStride(buffer_view) ?
                            (accessor.ByteStride(buffer_view) / sizeof(float)) :
                            (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                        break;
                    case AttributeType::Position:
                        if (accessor.type != TINYGLTF_TYPE_VEC3)
                            throw std::runtime_error("Position isn't vec3");
                        position_accessor = accessor;
                        data_position = data;
                        stride_position = accessor.ByteStride(buffer_view) ?
                            (accessor.ByteStride(buffer_view) / sizeof(float)) :
                            (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3));
                        break;
                    case AttributeType::Texcoord0:
                        if (accessor.type != TINYGLTF_TYPE_VEC2)
                            throw std::runtime_error("Texcoord0 isn't vec2");
                        data_texcoord0 = data;
                        stride_texcoord0 = accessor.ByteStride(buffer_view) ?
                            (accessor.ByteStride(buffer_view) / sizeof(float)) :
                            (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                        break;
                    case AttributeType::Texcoord1:
                        if (accessor.type != TINYGLTF_TYPE_VEC2)
                            throw std::runtime_error("Texcoord1 isn't vec2");
                        data_texcoord1 = data;
                        stride_texcoord1 = accessor.ByteStride(buffer_view) ?
                            (accessor.ByteStride(buffer_view) / sizeof(float)) :
                            (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                        break;
                    case AttributeType::Texcoord2:
                        if (accessor.type != TINYGLTF_TYPE_VEC2)
                            throw std::runtime_error("Texcoord2 isn't vec2");
                        stride_texcoord2 = accessor.ByteStride(buffer_view) ?
                            (accessor.ByteStride(buffer_view) / sizeof(float)) :
                            (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2));
                        data_texcoord2 = data;
                        break;
                    case AttributeType::Color0:
                        new_primitive.has_vertex_coloring = true;
                        stride_color0 = accessor.ByteStride(buffer_view) ?
                            (accessor.ByteStride(buffer_view) / sizeof(float)) :
                            (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type));
                        data_color0 = data;
                        type_color0 = accessor.type;
                        break;
                    case AttributeType::Tangent:
                        stride_tangent = accessor.ByteStride(buffer_view) ?
                            (accessor.ByteStride(buffer_view) / sizeof(float)) :
                            (tinygltf::GetComponentSizeInBytes(accessor.componentType) * tinygltf::GetNumComponentsInType(accessor.type));
                        data_tangent = data;
                        break;
                    default:
                        break;
                    }
                }

                if (!data_position)
                    throw std::runtime_error("glTF primitive must have a position attribute");
                if (position_accessor.minValues.size() != 3 || position_accessor.maxValues.size() != 3)
                    throw std::runtime_error("Position accessor doesn't contain min or max values");

                new_primitive.bounding_box.min = glm::make_vec3<double>(position_accessor.minValues.data());
                new_primitive.bounding_box.max = glm::make_vec3<double>(position_accessor.maxValues.data());
                new_primitive.bounding_sphere = Sphere::bounding_sphere_from_bounding_box(new_primitive.bounding_box);
                m_primitives.push_back(std::move(new_primitive));

                for (size_t i = 0; i < position_accessor.count; i++) {
                    Vertex vertex{};
                    vertex.position = glm::make_vec3(&data_position[i * stride_position]);
                    vertex.normal = data_normal ? glm::normalize(glm::make_vec3(&data_normal[i * stride_normal])) : glm::vec3(0.0f);
                    vertex.texture_coord0 = data_texcoord0 ? glm::make_vec2(&data_texcoord0[i * stride_texcoord0]) : glm::vec2(0.0f);
                    //vertex.texture_coord1 = data_texcoord1 ? glm::make_vec2(&data_texcoord1[i * stride_texcoord1]) : glm::vec2(0.0f);

                    if (data_color0) {
                        if (type_color0 == TINYGLTF_TYPE_VEC3) {
                            vertex.color0 = glm::vec4{
                                data_color0[i * stride_color0 + 0],
                                data_color0[i * stride_color0 + 1],
                                data_color0[i * stride_color0 + 2],
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

        std::copy(node.children.begin(), node.children.end(), std::back_inserter(new_node.children));
        m_nodes.push_back(std::move(new_node));
    }
}

void glTFModel::debug_print_node(const Node &node, uint32_t indent) const {
    LOG_INFO("(glTF)({: >{}} NODE: ", "", indent);
    if (node.mesh.has_value()) {
        LOG_INFO("(glTF){: >{}}    MESH #{}: ", "", indent, node.mesh.value());
        const auto &mesh = m_meshes.at(node.mesh.value());

        for (const auto &primitive_idx : mesh.primitives) {
            LOG_INFO("(glTF){: >{}}        PRIMITIVE #{}: ", "", indent, primitive_idx);
            const auto &primitive = m_primitives.at(primitive_idx);
            if (primitive.material.has_value())
                LOG_INFO("(glTF){: >{}}        HAS MATERIAL: #{}", "", indent, primitive.material.value());
            LOG_INFO("(glTF){: >{}}        VERTEX COUNT = {}", "", indent, primitive.indices.size());
        }
    }

    for (size_t child_idx : node.children)
        debug_print_node(m_nodes[child_idx], indent + 4);
}

void glTFModel::debug_print() const {
    LOG_INFO("(glTF) Size of nodes: {}", m_nodes.size());
    for (size_t node_idx : m_root_nodes)
        debug_print_node(m_nodes[node_idx], 0);
}

const std::vector<Vertex> &glTFModel::get_vertices() const {
    return m_vertices;
}

const std::vector<size_t> &glTFModel::get_root_nodes() const {
    return m_root_nodes;
}

size_t glTFModel::get_root_node_count() const {
    return m_root_nodes.size();
}

size_t glTFModel::get_node_count() const {
    return m_nodes.size();
}

size_t glTFModel::get_primitive_count() const {
    return m_primitives.size();
}

size_t glTFModel::get_texture_count() const {
    return m_textures.size();
}

size_t glTFModel::get_sampler_count() const {
    return m_samplers.size();
}

size_t glTFModel::get_image_count() const {
    return m_images.size();
}

size_t glTFModel::get_animation_count() const {
    return m_animations.size();
}

const glTFModel::Node &glTFModel::get_node(size_t index) const {
    return m_nodes.at(index);
}

const glTFModel::Primitive &glTFModel::get_primitive(size_t index) const {
    return m_primitives.at(index);
}

const glTFModel::Mesh &glTFModel::get_mesh(size_t index) const {
    return m_meshes.at(index);
}

const glTFModel::Material &glTFModel::get_material(size_t index) const {
    return m_materials.at(index);
}

const glTFModel::Texture &glTFModel::get_texture(size_t index) const {
    return m_textures.at(index);
}

const glTFModel::Image &glTFModel::get_image(size_t index) const {
    return m_images.at(index);
}

const glTFModel::Animation &glTFModel::get_animation(size_t index) const {
    return m_animations.at(index);
}

const glTFModel::Sampler &glTFModel::get_sampler(size_t index) const {
    return m_samplers.at(index);
}

}
