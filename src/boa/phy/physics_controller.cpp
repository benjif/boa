#include "boa/ecs/ecs.h"
#include "boa/utl/macros.h"
#include "boa/phy/physics_controller.h"
#include "boa/gfx/asset/asset_manager.h"
#include "boa/gfx/asset/asset.h"
#include "boa/gfx/linear.h"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/matrix_decompose.hpp"

namespace boa::phy {

PhysicsController::PhysicsController(boa::gfx::AssetManager &asset_manager)
    : m_asset_manager(asset_manager), m_enabled(false)
{
    m_collision_config = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collision_config.get());
    m_overlapping_pair_cache = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
    m_dynamics_world = std::make_unique<btDiscreteDynamicsWorld>(m_dispatcher.get(), m_overlapping_pair_cache.get(), m_solver.get(), m_collision_config.get());

    m_dynamics_world->setGravity(btVector3(0, -9.806, 0));
}

PhysicsController::~PhysicsController() {
    m_deletion_queue.flush();
}

void PhysicsController::add_entity(uint32_t e_id, float f_mass) {
    LOG_INFO("(Physics) Adding entity {} to dynamics world with mass of {}", e_id, f_mass);

    auto &entity_group = ecs::EntityGroup::get();
    assert(entity_group.has_component<gfx::Transformable>(e_id));
    assert(entity_group.has_component<gfx::Renderable>(e_id));

    auto &model = m_asset_manager.get_model(entity_group.get_component<gfx::Renderable>(e_id).model_id);
    auto &transform = entity_group.get_component<gfx::Transformable>(e_id);

    gfx::Box on_origin = model.bounding_box;

    on_origin.transform(transform.transform_matrix);

    btCollisionShape *ground_shape = new btBoxShape(btVector3(fabs(on_origin.max.x - on_origin.min.x) / 2,
                                                              fabs(on_origin.max.y - on_origin.min.y) / 2,
                                                              fabs(on_origin.max.z - on_origin.min.z) / 2));

    glm::vec3 center = on_origin.center();

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

    entity_group.enable_and_make<Physical>(e_id,
                                           ground_shape,
                                           motion_state,
                                           body,
                                           e_id);

    body->setUserPointer(&entity_group.get_component<Physical>(e_id));

    m_dynamics_world->addRigidBody(body);

    m_deletion_queue.enqueue([=]() {
        delete ground_shape;
        delete body->getMotionState();
        m_dynamics_world->removeCollisionObject(body);
        delete body;
    });
}

float PhysicsController::get_entity_mass(uint32_t e_id) const {
    auto &entity_group = ecs::EntityGroup::get();
    assert(entity_group.has_component<Physical>(e_id));
    float inv_mass = entity_group.get_component<Physical>(e_id).rigid_body->getInvMass();
    if (inv_mass == 0.0f)
        return 0.0f;
    return 1.0f / inv_mass;
}

void PhysicsController::update(float time_change) {
    if (!m_enabled)
        return;

    m_dynamics_world->stepSimulation(time_change);

    auto &entity_group = ecs::EntityGroup::get();
    entity_group.for_each_entity_with_component<Physical, gfx::Transformable, gfx::Renderable>([&](uint32_t e_id) {
        auto &physical = entity_group.get_component<Physical>(e_id);
        auto &transform = entity_group.get_component<gfx::Transformable>(e_id);

        btCollisionObject *obj = (btCollisionObject *)physical.rigid_body;

        btTransform trans;
        if (physical.rigid_body->getMotionState())
            physical.rigid_body->getMotionState()->getWorldTransform(trans);
        else
            trans = obj->getWorldTransform();

        transform.translation = glm::vec3(float(trans.getOrigin().getX()),
                                          float(trans.getOrigin().getY()),
                                          float(trans.getOrigin().getZ()));
        transform.orientation = glm::quat(float(trans.getRotation().getW()),
                                          float(trans.getRotation().getX()),
                                          float(trans.getRotation().getY()),
                                          float(trans.getRotation().getZ()));
        transform.update();

        return Iteration::Continue;
    });
}

std::optional<uint32_t> PhysicsController::raycast_cursor_position(uint32_t screen_w, uint32_t screen_h,
                                                uint32_t cursor_x, uint32_t cursor_y,
                                                const glm::mat4 &view_projection,
                                                float length) {
    glm::mat4 inverse_view_projection = glm::inverse(view_projection);

    glm::vec4 start_device{
        ((float)cursor_x / (float)screen_w - 0.5f) * 2.0f,
        ((float)cursor_y / (float)screen_h - 0.5f) * 2.0f,
        -1.0f,
        1.0f,
    };
    glm::vec4 end_device{
        ((float)cursor_x / (float)screen_w - 0.5f) * 2.0f,
        ((float)cursor_y / (float)screen_h - 0.5f) * 2.0f,
        0.0f,
        1.0f,
    };

    glm::vec4 start_scene = inverse_view_projection * start_device;
    start_scene /= start_scene.w;
    glm::vec4 end_scene = inverse_view_projection * end_device;
    end_scene /= end_scene.w;

    glm::vec4 ray_direction = glm::normalize(end_scene - start_scene);
    glm::vec4 ray_end = start_scene + ray_direction * length;

    btCollisionWorld::ClosestRayResultCallback ray_callback(
        btVector3(start_scene.x, start_scene.y, start_scene.z),
        btVector3(ray_end.x, ray_end.y, ray_end.z));
    m_dynamics_world->rayTest(
        btVector3(start_scene.x, start_scene.y, start_scene.z),
        btVector3(ray_end.x, ray_end.y, ray_end.z),
        ray_callback);

    if (ray_callback.hasHit()) {
        Physical *object = (Physical *)ray_callback.m_collisionObject->getUserPointer();
        return object->e_id;
    }

    return std::nullopt;
}

void PhysicsController::enable_physics() {
    m_enabled = true;
}

void PhysicsController::disable_physics() {
    m_enabled = false;
}

void PhysicsController::set_gravity(float gravity) {
    m_dynamics_world->setGravity(btVector3(0, gravity, 0));
}

float PhysicsController::get_gravity() const {
    return m_dynamics_world->getGravity().getY();
}

bool PhysicsController::is_physics_enabled() const {
    return m_enabled;
}

void PhysicsController::sync_physics_transform(uint32_t e_id) const {
    auto &entity_group = boa::ecs::EntityGroup::get();
    auto &transform = entity_group.get_component<boa::gfx::Transformable>(e_id);
    auto &physical = entity_group.get_component<Physical>(e_id);

    glm::vec3 scale, translation, skew;
    glm::quat orientation;
    glm::vec4 perspective;

    // matrix decomposition is unnecessary (btTransform has a `setFromOpenGLMatrix`), but
    // this is the only way I could get it to work.
    glm::decompose(transform.transform_matrix, scale, orientation, translation, skew, perspective);
    orientation = glm::conjugate(orientation);

    btTransform bt_transform;
    bt_transform.setIdentity();
    bt_transform.setOrigin(btVector3(translation.x, translation.y, translation.z));
    bt_transform.setRotation(btQuaternion(orientation.x,
                                          orientation.y, 
                                          orientation.z,
                                          orientation.w));

    physical.ground_shape->setLocalScaling(btVector3(scale.x, scale.y, scale.z));
    physical.rigid_body->setCenterOfMassTransform(bt_transform);
    m_dynamics_world->getCollisionWorld()->updateSingleAabb(physical.rigid_body);
}

glm::dvec3 PhysicsController::get_linear_velocity(uint32_t e_id) const {
    auto &entity_group = boa::ecs::EntityGroup::get();
    auto &physical = entity_group.get_component<Physical>(e_id);

    const btVector3 &linear_velocity = physical.rigid_body->getLinearVelocity();

    return glm::dvec3{ linear_velocity.getX(), linear_velocity.getY(), linear_velocity.getZ() };
}

glm::dvec3 PhysicsController::get_angular_velocity(uint32_t e_id) const {
    auto &entity_group = boa::ecs::EntityGroup::get();
    auto &physical = entity_group.get_component<Physical>(e_id);

    const btVector3 &angular_velocity = physical.rigid_body->getAngularVelocity();

    return glm::dvec3{ angular_velocity.getX(), angular_velocity.getY(), angular_velocity.getZ() };
}

}
