#ifndef BOA_PHY_PHYSICS_CONTROLLER_H
#define BOA_PHY_PHYSICS_CONTROLLER_H

#include "btBulletDynamicsCommon.h"
#include "glm/glm.hpp"
#include "boa/utl/macros.h"
#include "boa/phy/debug_drawer.h"
#include <unordered_set>
#include <memory>
#include <utility>
#include <functional>

namespace boa::gfx {
class AssetManager;
class Renderer;
}

namespace boa::phy {

struct PairHash {
    template <typename T1, typename T2>
    std::size_t operator()(std::pair<T1, T2> const &p) const {
        std::size_t h1 = std::hash<T1>()(p.first);
        std::size_t h2 = std::hash<T1>()(p.second);
        return h1 ^ h2;
    }
};

class BulletDebugDrawer;

struct Physical {
    Physical()
        : ground_shape(nullptr),
          motion_state(nullptr),
          rigid_body(nullptr)
    {
    }

    Physical(btCollisionShape *gs, btDefaultMotionState *ms, btRigidBody *rb, glm::vec3 &&center, uint32_t id)
        : ground_shape(gs),
          motion_state(ms),
          rigid_body(rb),
          on_origin_center(std::move(center)),
          e_id(id)
    {
    }

    btCollisionShape *ground_shape;
    btDefaultMotionState *motion_state;
    btRigidBody *rigid_body;
    glm::vec3 on_origin_center;
    uint32_t e_id;
};

class PhysicsController {
    REMOVE_COPY_AND_ASSIGN(PhysicsController);
public:
    PhysicsController(boa::gfx::AssetManager &asset_manager);
    ~PhysicsController();

    void add_entity(uint32_t e_id, float f_mass);
    void remove_entity(uint32_t e_id);
    float get_entity_mass(uint32_t e_id) const;
    void set_entity_mass(uint32_t e_id, float mass) const;
    void update(float time_change);

    void set_gravity(float gravity);
    float get_gravity() const;

    void enable_physics();
    void disable_physics();
    bool is_physics_enabled() const;

    void sync_physics_transform(uint32_t e_id) const;
    void reset_object_velocity(uint32_t e_id) const;

    glm::dvec3 get_linear_velocity(uint32_t e_id) const;
    glm::dvec3 get_angular_velocity(uint32_t e_id) const;

    std::optional<uint32_t> raycast_cursor_position(uint32_t screen_w, uint32_t screen_h,
                                                    uint32_t cursor_x, uint32_t cursor_y,
                                                    const glm::mat4 &view_projection,
                                                    float length);

    void set_entity_deletion_cutoff(double max);

    void set_collision_callback(std::function<void(uint32_t, uint32_t)> callback);

    void enable_debug_drawing(boa::gfx::Renderer &renderer);
    void debug_draw() const;
    void debug_reset() const;

private:
    boa::gfx::AssetManager &m_asset_manager;

    std::unique_ptr<btDefaultCollisionConfiguration> m_collision_config;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_overlapping_pair_cache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamics_world;

    std::unique_ptr<BulletDebugDrawer> m_debug_drawer;

    bool m_enabled;

    std::optional<double> m_entity_deletion_cutoff;
    std::unordered_set<std::pair<uint32_t, uint32_t>, PairHash> m_present_manifolds;

    std::function<void(uint32_t, uint32_t)> m_collision_callback;
};

}

#endif
