#include "boa/phy/physics_controller.h"
#include "boa/gfx/asset/asset.h"
#include "boa/gfx/linear.h"
#include "boa/ecs/ecs.h"
#include "boa/macros.h"

namespace boa::phy {

PhysicsController::PhysicsController()
{
    m_collision_config = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collision_config.get());
    m_overlapping_pair_cache = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_dynamics_world = std::make_unique<btDiscreteDynamicsWorld>(m_dispatcher.get(), m_overlapping_pair_cache.get(), m_solver.get(), m_collision_config.get());

    m_dynamics_world->setGravity(btVector3(0, -10, 0));
}

PhysicsController::~PhysicsController() {
    m_deletion_queue.flush();
}

void PhysicsController::add_entity(uint32_t e_id, float f_mass) {
    LOG_INFO("(Physics) Adding entity {} to dynamics world with mass of {}", e_id, f_mass);

    auto &entity_group = ecs::EntityGroup::get();
    assert(entity_group.has_component<gfx::Transformable>(e_id));
    assert(entity_group.has_component<gfx::RenderableModel>(e_id));

    auto &model = entity_group.get_component<gfx::RenderableModel>(e_id);
    auto &transform = entity_group.get_component<gfx::Transformable>(e_id);

    gfx::Box on_origin = model.bounding_box;
    //on_origin.min = transform.transform_matrix * glm::vec4(on_origin.min, 1.f);
    //on_origin.max = transform.transform_matrix * glm::vec4(on_origin.max, 1.f);

    on_origin.transform(transform.transform_matrix);

    glm::vec3 center = on_origin.center();
    on_origin.center_on_origin();

    btCollisionShape *ground_shape = new btBoxShape(btVector3(fabs(on_origin.max.x - on_origin.min.x) / 2,
                                                    fabs(on_origin.max.y - on_origin.min.y) / 2,
                                                    fabs(on_origin.max.z - on_origin.min.z) / 2));
    //ground_shape->setLocalScaling(btVector3(transform.scale.x, transform.scale.y, transform.scale.z));

    btTransform ground_transform;
    ground_transform.setIdentity();
    ground_transform.setOrigin(btVector3(center.x, center.y, center.z));

    btScalar mass(f_mass);
    bool is_dynamic = (f_mass != 0.f);

    btVector3 local_inertia(0, 0, 0);
    if (is_dynamic)
        ground_shape->calculateLocalInertia(mass, local_inertia);

    btDefaultMotionState *motion_state = new btDefaultMotionState(ground_transform);
    btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state, ground_shape, local_inertia);
    btRigidBody *body = new btRigidBody(rb_info);

    //LOG_INFO("Linear sleeping threshold: {}", body->getLinearSleepingThreshold());
    //LOG_INFO("Angular sleeping threshold: {}", body->getAngularSleepingThreshold());

    //body->setSleepingThresholds(0.2f, 0.2f);

    m_dynamics_world->addRigidBody(body);

    entity_group.enable_and_make<Physical>(e_id,
                                           ground_shape, motion_state, body);

    m_deletion_queue.enqueue([=]() {
        delete ground_shape;
        delete body->getMotionState();
        m_dynamics_world->removeCollisionObject(body);
        delete body;
    });
}

void PhysicsController::update(float time_change) {
    m_dynamics_world->stepSimulation(time_change);

    auto &entity_group = ecs::EntityGroup::get();
    entity_group.for_each_entity_with_component<Physical, gfx::Transformable, gfx::RenderableModel>([&](uint32_t e_id) {
        auto &physical = entity_group.get_component<Physical>(e_id);
        auto &transform = entity_group.get_component<gfx::Transformable>(e_id);

        btCollisionObject *obj = (btCollisionObject *)physical.rigid_body;

        btTransform trans;
        if (physical.rigid_body->getMotionState()) {
            physical.rigid_body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = obj->getWorldTransform();
        }

        transform.translation = glm::vec3(float(trans.getOrigin().getX()),
                                          float(trans.getOrigin().getY()),
                                          float(trans.getOrigin().getZ()));
        transform.orientation = glm::quat(float(trans.getRotation().getX()),
                                          float(trans.getRotation().getY()),
                                          float(trans.getRotation().getZ()),
                                          float(trans.getRotation().getW()));
        transform.update();

        //LOG_INFO("{}, {}, {}",
                 //float(trans.getOrigin().getX()), float(trans.getOrigin().getY()), float(trans.getOrigin().getZ()));

        return Iteration::Continue;
    });
}

}
