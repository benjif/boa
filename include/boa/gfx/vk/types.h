#ifndef BOA_GFX_VK_TYPES_H
#define BOA_GFX_VK_TYPES_H

#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"

namespace boa::gfx {

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