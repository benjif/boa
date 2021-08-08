#include "boa/ecs/ecs.h"
#include "boa/utl/macros.h"
#include "boa/phy/physics_controller.h"
#include "boa/phy/bullet_glm.h"
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
    auto &entity_group = boa::ecs::EntityGroup::get();
    entity_group.for_each_entity_with_component<Physical>([&](uint32_t e_id) {
        auto &physical = entity_group.get_component<Physical>(e_id);
        if (physical.rigid_body && physical.rigid_body->getMotionState())
            delete physical.rigid_body->getMotionState();
        m_dynamics_world->removeCollisionObject(physical.rigid_body);
        delete physical.ground_shape;
        delete physical.rigid_body;

        return Iteration::Continue;
    });
}

void PhysicsController::add_entity(uint32_t e_id, float f_mass) {
    LOG_INFO("(Physics) Adding entity {} to dynamics world with mass of {}", e_id, f_mass);

    auto &entity_group = ecs::EntityGroup::get();
    assert(entity_group.has_component<gfx::Transformable>(e_id));
    assert(entity_group.has_component<gfx::Renderable>(e_id));

    auto &model = m_asset_manager.get_model(entity_group.get_component<gfx::Renderable>(e_id).model_id);
    auto &transform = entity_group.get_component<gfx::Transformable>(e_id);

    gfx::Box on_origin = model.bounding_box;
    glm::vec3 size{
        fabs(on_origin.max.x - on_origin.min.x) / 2,
        fabs(on_origin.max.y - on_origin.min.y) / 2,
        fabs(on_origin.max.z - on_origin.min.z) / 2,
    };

    btCollisionShape *ground_shape = new btBoxShape(glm_to_bullet(size));

    glm::vec3 center = on_origin.center();

    // offset by center because bullet3 uses the center of mass as the origin
    btTransform ground_transform = glm_to_bullet(glm::translate(transform.transform_matrix, center));

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
                                           std::move(center),
                                           e_id);

    body->setUserPointer(&entity_group.get_component<Physical>(e_id));

    m_dynamics_world->addRigidBody(body);
}

void PhysicsController::remove_entity(uint32_t e_id) {
    auto &entity_group = ecs::EntityGroup::get();
    if (!entity_group.has_component<Physical>(e_id))
        return;

    auto &physical = entity_group.get_component<Physical>(e_id);
    if (physical.rigid_body && physical.rigid_body->getMotionState())
        delete physical.rigid_body->getMotionState();
    m_dynamics_world->removeCollisionObject(physical.rigid_body);
    delete physical.ground_shape;
    delete physical.rigid_body;

    entity_group.disable<Physical>(e_id);
}

float PhysicsController::get_entity_mass(uint32_t e_id) const {
    auto &entity_group = ecs::EntityGroup::get();
    assert(entity_group.has_component<Physical>(e_id));
    float inv_mass = entity_group.get_component<Physical>(e_id).rigid_body->getInvMass();
    if (inv_mass == 0.0f)
        return 0.0f;
    return 1.0f / inv_mass;
}

void PhysicsController::set_entity_mass(uint32_t e_id, float mass) const {
    auto &entity_group = ecs::EntityGroup::get();
    if (!entity_group.has_component<Physical>(e_id))
        return;

    auto &physical = entity_group.get_component<Physical>(e_id);
    physical.rigid_body->setMassProps(mass, btVector3(0, 0, 0));

    if (mass == 0.f) {
        physical.rigid_body->setCollisionFlags(physical.rigid_body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
        physical.rigid_body->setLinearVelocity(btVector3(0, 0, 0));
        physical.rigid_body->setAngularVelocity(btVector3(0, 0, 0));
    } else {
        physical.rigid_body->setCollisionFlags(physical.rigid_body->getCollisionFlags() & ~btCollisionObject::CF_STATIC_OBJECT);
    }
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

        btTransform trans = obj->getWorldTransform();

        // PERF: decomposition/recomposition shouldn't be necessary but it works for now
        transform.transform_matrix = bullet_to_glm(trans);
        transform.transform_matrix = glm::translate(transform.transform_matrix, -physical.on_origin_center);
        transform.decompose();

        if (m_entity_deletion_cutoff.has_value() && glm::length(transform.translation) > m_entity_deletion_cutoff.value()) {
            remove_entity(e_id);
            entity_group.delete_entity(e_id);
        }

        return Iteration::Continue;
    });

    std::unordered_set<std::pair<uint32_t, uint32_t>, PairHash> tmp_present_manifolds;
    for (size_t i = 0; i < m_dispatcher->getNumManifolds(); i++) {
        btPersistentManifold *contact_manifold = m_dispatcher->getManifoldByIndexInternal(i);
        const btCollisionObject *object_a = contact_manifold->getBody0();
        const btCollisionObject *object_b = contact_manifold->getBody1();

        const Physical *physical_a = static_cast<const Physical *>(object_a->getUserPointer());
        const Physical *physical_b = static_cast<const Physical *>(object_b->getUserPointer());

        std::pair<uint32_t, uint32_t> manifold_entities(physical_a->e_id, physical_b->e_id);

        tmp_present_manifolds.insert(manifold_entities);
        if (m_present_manifolds.count(manifold_entities) == 0 && m_collision_callback)
            m_collision_callback(physical_a->e_id, physical_b->e_id);
    }

    m_present_manifolds = std::move(tmp_present_manifolds);
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
        glm_to_bullet(start_scene),
        glm_to_bullet(ray_end));

    m_dynamics_world->rayTest(
        glm_to_bullet(start_scene),
        glm_to_bullet(ray_end),
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

    btTransform bt_transform = glm_to_bullet(glm::translate(transform.transform_matrix, physical.on_origin_center));

    //glm::vec3 scale = transform.scale / bullet_to_glm(physical.rigid_body->getCollisionShape()->getLocalScaling());

    physical.rigid_body->getCollisionShape()->setLocalScaling(glm_to_bullet(transform.scale));
    physical.rigid_body->setCenterOfMassTransform(bt_transform);
    physical.rigid_body->activate(true);

    m_dynamics_world->computeOverlappingPairs();
    m_dynamics_world->synchronizeSingleMotionState(physical.rigid_body);
    m_dynamics_world->updateSingleAabb(physical.rigid_body);
    //m_dynamics_world->performDiscreteCollisionDetection();
    //m_dynamics_world->updateAabbs();
}

glm::dvec3 PhysicsController::get_linear_velocity(uint32_t e_id) const {
    auto &entity_group = boa::ecs::EntityGroup::get();
    auto &physical = entity_group.get_component<Physical>(e_id);

    const btVector3 &linear_velocity = physical.rigid_body->getLinearVelocity();

    return bullet_to_glm(linear_velocity);
}

glm::dvec3 PhysicsController::get_angular_velocity(uint32_t e_id) const {
    auto &entity_group = boa::ecs::EntityGroup::get();
    auto &physical = entity_group.get_component<Physical>(e_id);

    const btVector3 &angular_velocity = physical.rigid_body->getAngularVelocity();

    return bullet_to_glm(angular_velocity);
}

void PhysicsController::set_entity_deletion_cutoff(double max) {
    m_entity_deletion_cutoff = max;
}

void PhysicsController::enable_debug_drawing(boa::gfx::Renderer &renderer) {
    m_debug_drawer = std::make_unique<BulletDebugDrawer>(renderer);
    m_debug_drawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    m_dynamics_world->setDebugDrawer(static_cast<btIDebugDraw *>(m_debug_drawer.get()));
}

void PhysicsController::debug_draw() const {
    if (m_debug_drawer) {
        m_dynamics_world->debugDrawWorld();
        m_debug_drawer->upload();
    }
}

void PhysicsController::debug_reset() const {
    m_debug_drawer->reset();
}

void PhysicsController::set_collision_callback(std::function<void(uint32_t, uint32_t)> callback) {
    m_collision_callback = callback;
}

}
