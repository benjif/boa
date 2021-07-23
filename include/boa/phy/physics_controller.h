#ifndef BOA_PHY_PHYSICS_CONTROLLER_H
#define BOA_PHY_PHYSICS_CONTROLLER_H

#include "btBulletDynamicsCommon.h"
#include "boa/deletion_queue.h"
#include <unordered_set>
#include <memory>

namespace boa::gfx {
class AssetManager;
}

namespace boa::phy {

// component
struct Physical {
    Physical()
        : ground_shape(nullptr),
          motion_state(nullptr),
          rigid_body(nullptr)
    {
    }

    Physical(btCollisionShape *gs, btDefaultMotionState *ms, btRigidBody *rb)
        : ground_shape(gs),
          motion_state(ms),
          rigid_body(rb)
    {
    }

    btCollisionShape *ground_shape;
    btDefaultMotionState *motion_state;
    btRigidBody *rigid_body;
};

class PhysicsController {
public:
    PhysicsController(boa::gfx::AssetManager &asset_manager);
    ~PhysicsController();

    void add_entity(uint32_t e_id, float f_mass);
    void update(float time_change);

private:
    boa::gfx::AssetManager &m_asset_manager;

    std::unique_ptr<btDefaultCollisionConfiguration> m_collision_config;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_overlapping_pair_cache;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamics_world;

    DeletionQueue m_deletion_queue;
};

}

#endif
