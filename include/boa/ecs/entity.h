#ifndef BOA_ECS_ENTITY_H
#define BOA_ECS_ENTITY_H

#include <stdint.h>
#include <bitset>

namespace boa {

const size_t MAX_COMPONENTS = 64;

struct EntityMeta {
    uint32_t id;
    std::bitset<MAX_COMPONENTS> components;
};

}

#endif