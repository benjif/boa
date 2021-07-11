#ifndef BOA_GFX_ASSET_LINEAR_TYPES_H
#define BOA_GFX_ASSET_LINEAR_TYPES_H

#include "glm/glm.hpp"
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

}

template <> struct fmt::formatter<boa::gfx::Vertex> {
    constexpr auto parse(format_parse_context &ctx) {
        auto it = ctx.begin();
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const boa::gfx::Vertex &v, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "({: f}, {: f}, {: f})", v.position.x, v.position.y, v.position.z);
    }
};

template <> struct fmt::formatter<boa::gfx::Box> {
    constexpr auto parse(format_parse_context &ctx) {
        auto it = ctx.begin();
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const boa::gfx::Box &b, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "{} => {}", b.min, b.max);
    }
};

#endif