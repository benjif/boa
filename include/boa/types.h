#ifndef BOA_TYPES_H
#define BOA_TYPES_H

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

namespace boa {

struct VmaBuffer {
    vk::Buffer buffer;
    VmaAllocation allocation;
};

struct VmaImage {
    vk::Image image;
    VmaAllocation allocation;
};

}

#endif