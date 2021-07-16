#ifndef BOA_GFX_LINEAR_TYPES_H
#define BOA_GFX_LINEAR_TYPES_H

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include <string>

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

struct Box {
    glm::dvec3 min, max;
};

struct Sphere {
    static Sphere bounding_sphere_from_bounding_box(const Box &box);
    glm::vec3 center;
    float radius;
};

struct Frustum {
    void update(glm::mat4 projection);
    bool is_sphere_within(const glm::vec3 &center, float radius) const;
    bool is_box_within(const Box &box) const;

    std::array<glm::vec4, 6> planes;
};

}

template <> struct fmt::formatter<glm::vec3> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const glm::vec3 &v, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "({: f}, {: f}, {: f})", v.x, v.y, v.z);
    }
};

template <> struct fmt::formatter<glm::vec4> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const glm::vec4 &v, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "({: f}, {: f}, {: f}, {: f})", v.x, v.y, v.z, v.w);
    }
};

template <> struct fmt::formatter<boa::gfx::Vertex> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const boa::gfx::Vertex &v, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{}", v.position);
    }
};

template <> struct fmt::formatter<boa::gfx::Box> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const boa::gfx::Box &b, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{} => {}", b.min, b.max);
    }
};

#endif