#include "boa/utl/macros.h"
#include "boa/phy/debug_drawer.h"
#include "boa/phy/bullet_glm.h"
#include "boa/gfx/renderer.h"
#include "glm/glm.hpp"

namespace boa::phy {

BulletDebugDrawer::BulletDebugDrawer(boa::gfx::Renderer &renderer)
    : boa::gfx::DebugDrawer(renderer),
      m_renderer(renderer)
{
    m_renderer.add_debug_drawer((boa::gfx::DebugDrawer *)this);
    // we only use one color currently
    set_color(glm::vec3{ 0.0f, 1.0f, 0.0f });
}

BulletDebugDrawer::~BulletDebugDrawer() {
}

void BulletDebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &to_color) {
    add_line(bullet_to_glm(from), bullet_to_glm(to));
}

void BulletDebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
    add_line(bullet_to_glm(from), bullet_to_glm(to));
}

void BulletDebugDrawer::drawSphere(const btVector3 &p, btScalar radius, const btVector3 &color) {
    TODO();
}

void BulletDebugDrawer::drawTriangle(const btVector3 &a, const btVector3 &b, const btVector3 &c, const btVector3 &color, btScalar alpha) {
    TODO();
}

void BulletDebugDrawer::drawContactPoint(const btVector3 &point_on_b, const btVector3 &normal_on_b, btScalar distance, int life_time, const btVector3 &color) {
    TODO();
}

void BulletDebugDrawer::reportErrorWarning(const char *warning_string) {
    LOG_WARN("(Physics) Warning from debug drawer: '{}'", warning_string);
}

void BulletDebugDrawer::draw3dText(const btVector3 &location, const char *text_string) {
    TODO();
}

void BulletDebugDrawer::setDebugMode(int debug_mode) {
    m_debug_mode = debug_mode;
}

void BulletDebugDrawer::upload() {
    boa::gfx::DebugDrawer::upload();
}

void BulletDebugDrawer::reset() {
    boa::gfx::DebugDrawer::reset();
}

}
