#ifndef BOA_PHY_DEBUG_DRAWER_H
#define BOA_PHY_DEBUG_DRAWER_H

#include "boa/gfx/debug_drawer.h"
#include "LinearMath/btIDebugDraw.h"

namespace boa::gfx {
class Renderer;
}

namespace boa::phy {

class BulletDebugDrawer : public btIDebugDraw, public boa::gfx::DebugDrawer {
public:
    BulletDebugDrawer(boa::gfx::Renderer &renderer);
    virtual ~BulletDebugDrawer();

    virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &to_color);
    virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color);
    virtual void drawSphere(const btVector3 &p, btScalar radius, const btVector3 &color);
    virtual void drawTriangle(const btVector3 &a, const btVector3 &b, const btVector3 &c, const btVector3 &color, btScalar alpha);
    virtual void drawContactPoint(const btVector3 &point_on_b, const btVector3 &normal_on_b, btScalar distance, int life_time, const btVector3 &color);
    virtual void reportErrorWarning(const char *warning_string);
    virtual void draw3dText(const btVector3 &location, const char *text_string);

    virtual void setDebugMode(int debug_mode);
    virtual int  getDebugMode() const { return m_debug_mode; }

    void upload();
    void reset();

private:
    boa::gfx::Renderer &m_renderer;
    int m_debug_mode;
};

}

#endif
