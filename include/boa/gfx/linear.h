#ifndef BOA_GFX_LINEAR_H
#define BOA_GFX_LINEAR_H

#include "glm/glm.hpp"
#include "glm/matrix.hpp"
#include "glm/gtc/quaternion.hpp"
#include <vulkan/vulkan.hpp>
#include <fmt/format.h>
#include <string>

namespace boa::gfx {

struct Transformable {
    Transformable() {}

    Transformable(glm::quat &&ori, glm::vec3 &&trn, glm::vec3 && scl)
        : orientation(std::move(ori)),
          translation(std::move(trn)),
          scale(std::move(scl))
    {
        update();
    }

    glm::mat4 transform_matrix{ 1.0f };
    glm::quat orientation{ 0.0f, 0.0f, 0.0f, 0.0f };
    glm::vec3 translation{ 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    void update();
};

struct Vertex {
    glm::vec3 position, normal;
    glm::vec4 color0;
    glm::vec2 texture_coord0;

    // others?
    // texcoord1, tangent, joint0, weights0

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

struct Box {
    glm::vec3 min{ 0.0f }, max{ 0.0f };

    glm::vec3 center() const;
    void center_on_origin();
    void transform(const glm::mat4 &);
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

template <> struct fmt::formatter<glm::mat4> {
    constexpr auto parse(format_parse_context &ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const glm::mat4 &m, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "\n{},\n{},\n{},\n{}\n", m[0], m[1], m[2], m[3]);
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
