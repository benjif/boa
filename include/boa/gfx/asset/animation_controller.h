#ifndef BOA_GFX_ASSET_ANIMATION_CONTROLLER_H
#define BOA_GFX_ASSET_ANIMATION_CONTROLLER_H

#include "boa/macros.h"
#include <stdint.h>

namespace boa::gfx {

class glTFModel;

class AnimationController {
    REMOVE_COPY_AND_ASSIGN(AnimationController);
public:
    AnimationController() {}

    // TODO: return count of animations added?
    void load_animations(const boa::gfx::glTFModel &model, uint32_t e_id);
    void play_animation(uint32_t e_id, uint32_t animation_id, bool loop = false);
    void update(float time_change);
};

}

#endif