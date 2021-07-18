#ifndef BOA_GFX_LIGHTING_H
#define BOA_GFX_LIGHTING_H

#include "glm/glm.hpp"

namespace boa::gfx {

struct GlobalLight {
    GlobalLight() {}

    GlobalLight(glm::vec3 &&dir, glm::vec3 &&amb, glm::vec3 &&dif, glm::vec3 &&spc)
        : direction(std::move(dir)),
          ambient(std::move(amb)),
          diffuse(std::move(dif)),
          specular(std::move(spc))
    {
    }

    glm::vec3 direction;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

}

#endif