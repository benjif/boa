#include "boa/gfx/debug_drawer.h"
#include "boa/gfx/renderer.h"

namespace boa::gfx {

DebugDrawer::DebugDrawer(Renderer &renderer)
    : m_renderer(renderer)
{
    const size_t size = MAX_VERTICES * sizeof(SmallVertex);
    m_line_vertex_buffer = m_renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY);
}

DebugDrawer::~DebugDrawer() {
    vmaDestroyBuffer(m_renderer.get_allocator(), m_line_vertex_buffer.buffer, m_line_vertex_buffer.allocation);
}

void DebugDrawer::set_color(const glm::vec3 &color) {
    m_color = color;
}

glm::vec3 DebugDrawer::get_color() const {
    return m_color;
}

void DebugDrawer::add_line(const glm::vec3 &from, const glm::vec3 &to) {
    SmallVertex new_from{ .position = from };
    SmallVertex new_to{ .position = to };

    m_line_vertices.push_back(std::move(new_from));
    m_line_vertices.push_back(std::move(new_to));
}

void DebugDrawer::upload() {
    const size_t size = (m_line_vertices.size() * sizeof(SmallVertex)) > (MAX_VERTICES * sizeof(SmallVertex)) ?
        MAX_VERTICES * sizeof(SmallVertex) : m_line_vertices.size() * sizeof(SmallVertex);

    VmaBuffer staging_buffer = m_renderer.create_buffer(size, vk::BufferUsageFlagBits::eTransferSrc, VMA_MEMORY_USAGE_CPU_ONLY);

    void *data;
    vmaMapMemory(m_renderer.get_allocator(), staging_buffer.allocation, &data);
    memcpy(data, m_line_vertices.data(), size);
    vmaUnmapMemory(m_renderer.get_allocator(), staging_buffer.allocation);

    m_renderer.immediate_command([=](vk::CommandBuffer cmd) {
        vk::BufferCopy copy{ .srcOffset = 0, .dstOffset = 0, .size = size };
        cmd.copyBuffer(staging_buffer.buffer, m_line_vertex_buffer.buffer, copy);
    });

    vmaDestroyBuffer(m_renderer.get_allocator(), staging_buffer.buffer, staging_buffer.allocation);

    m_uploaded = true;
}

void DebugDrawer::reset() {
    m_line_vertices.clear();
    m_uploaded = false;
}

void DebugDrawer::record(vk::CommandBuffer cmd) {
    if (m_line_vertices.size() == 0 || !m_uploaded)
        return;

    vk::Buffer vertex_buffers[] = { m_line_vertex_buffer.buffer };
    vk::DeviceSize offsets[] = { 0 };
    cmd.bindVertexBuffers(0, 1, vertex_buffers, offsets);

    cmd.draw(m_line_vertices.size(), 1, 0, 0);
}

}
