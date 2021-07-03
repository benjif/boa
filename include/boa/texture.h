#ifndef BOA_TEXTURE_H
#define BOA_TEXTURE_H

#include "boa/types.h"
//#include "boa/renderer.h"
#include <vulkan/vulkan.hpp>

namespace boa {

class Renderer;

struct Texture {
    Texture();
    void load_from_file(Renderer *renderer, const char *path);

    VmaImage image;
    vk::ImageView image_view;

};

}

#endif