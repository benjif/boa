#include "boa/ngn/scripting_interface.h"
#include "boa/ecs/ecs.h"
#include "boa/gfx/linear.h"

extern "C" void set_entity_position(uint32_t e_id, float x, float y, float z) {
    auto &entity_group = boa::ecs::EntityGroup::get();

    if (!entity_group.has_component<boa::gfx::Transformable>(e_id))
        return;

    auto &transform = entity_group.get_component<boa::gfx::Transformable>(e_id);
    transform.translation.x = x;
    transform.translation.y = y;
    transform.translation.z = z;

    transform.update();
}
