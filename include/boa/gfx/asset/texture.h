#ifndef BOA_GFX_TEXTURE_H
#define BOA_GFX_TEXTURE_H

#include "boa/gfx/vk/types.h"
#include <stdint.h>
#include <vulkan/vulkan.hpp>

namespace boa::gfx {

class Renderer;

struct Texture {
    void load_from_file(Renderer *renderer, const char *path, bool mipmap = false);

    VmaImage image;
    vk::ImageView image_view;
    uint32_t mip_levels;
};

}

#endif