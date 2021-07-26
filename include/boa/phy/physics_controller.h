#ifndef BOA_PHY_PHYSICS_CONTROLLER_H
#define BOA_PHY_PHYSICS_CONTROLLER_H

#include "btBulletDynamicsCommon.h"
#include "glm/glm.hpp"
#include "boa/utl/macros.h"
#include "boa/utl/deletion_queue.h"
#include <unordered_set>
#include <memory>

namespace boa::gfx  { class AssetManager;   }

namespace boa::phy {

struct Physical {
    Physical()
        : ground_shape(nullptr),
          motion_state(nullptr),
          rigid_body(nullptr)
    {
    }

    Physical(btCollisionShape *gs, btDefaultMotionState *ms, btRigidBody *rb, uint32_t id)
        : ground_shape(gs),
          motion_state(ms),
          rigid_body(rb),
          e_id(id)
    {
    }

    btCollisionShape *ground_shape;
    btDefaultMotionState *motion_state;
    btRigidBody *rigid_body;
    uint32_t e_id;
};

class PhysicsController {
    REMOVE_COPY_AND_ASSIGN(PhysicsController);
public:
    PhysicsController(boa::gfx::AssetManager &asset_manager);
    ~PhysicsController();

    void add_entity(uint32_t e_id, float f_mass);
    float get_entity_mass(uint32_t e_id) const;
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

private:
    boa::gfx::AssetManager &m_asset_manager;

    std::unique_ptr<btDefaultCollisionConfiguration> m_collision_config;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_overlapping_pair_cache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamics_world;

    DeletionQueue m_deletion_queue;

    bool m_enabled;
};

}

#endif
