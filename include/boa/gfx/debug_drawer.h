#ifndef BOA_GFX_DEBUG_DRAWER_H
#define BOA_GFX_DEBUG_DRAWER_H

#include "glm/glm.hpp"
#include "boa/gfx/linear.h"
#include "boa/gfx/vk/types.h"
#include <vector>
#include <vulkan/vulkan.hpp>

namespace boa::gfx {

class Renderer;

class DebugDrawer {
public:
    static const size_t MAX_VERTICES = 10000;

    explicit DebugDrawer(Renderer &renderer);
    ~DebugDrawer();

    void set_color(const glm::vec3 &color);
    glm::vec3 get_color() const;
    void add_line(const glm::vec3 &from, const glm::vec3 &to);

    void upload();
    void reset();

    void record(vk::CommandBuffer cmd);

private:
    bool m_uploaded{ false };

    Renderer &m_renderer;
    std::vector<SmallVertex> m_line_vertices;
    glm::vec3 m_color;

    VmaBuffer m_line_vertex_buffer;
};

}

#endif
