#ifndef BOA_GFX_ANIMATION_H
#define BOA_GFX_ANIMATION_H

#include <stdint.h>

namespace boa::gfx { class glTFModel; }

namespace boa::gfx::anim {

class AnimationSystem {
public:
    void load_animations(const boa::gfx::glTFModel &model, uint32_t e_id);

private:
};

}

#endif