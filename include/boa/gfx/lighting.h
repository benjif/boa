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

    glm::vec3 direction;    float p0;
    glm::vec3 ambient;      float p1;
    glm::vec3 diffuse;      float p2;
    glm::vec3 specular;     float p3;
};

struct PointLight {
    PointLight() {}

    PointLight(glm::vec3 &&pos, glm::vec3 &&amb, glm::vec3 &&dif, glm::vec3 &&spc, float c, float l, float q)
        : position(std::move(pos)),
          ambient(std::move(amb)),
          diffuse(std::move(dif)),
          specular(std::move(spc)),
          constant(c),
          linear(l),
          quadratic(q)
    {
    }

    glm::vec3 position;     float constant;
    glm::vec3 ambient;      float linear;
    glm::vec3 diffuse;      float quadratic;
    glm::vec3 specular;     float p0;
};

}

#endif
