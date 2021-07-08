#ifndef BOA_ECS_COMPONENT_H
#define BOA_ECS_COMPONENT_H

#include <stdint.h>

namespace boa {

uint32_t component_count;

template <typename T>
uint32_t cpt_id() {
    static uint32_t component_id = component_count++;
    return component_id;
}

}

#endif