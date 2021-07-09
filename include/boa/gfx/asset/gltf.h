#ifndef BOA_GFX_ASSET_GLTF_H
#define BOA_GFX_ASSET_GLTF_H

#include <unordered_map>
#include <memory>

namespace boa::gfx::gltf {

enum class AttributeType {
    Normal,
    Position,
    Tangent,
    Texcoord0,
    Texcoord1,
    Texcoord2,
    Color0,
    Joint0,
    Weights0,
};

static const std::unordered_map<std::string, AttributeType> attribute_types = {
    { "NORMAL",     AttributeType::Normal       },
    { "POSITION",   AttributeType::Position     },
    { "TANGENT",    AttributeType::Tangent      },
    { "TEXCOORD_0", AttributeType::Texcoord0    },
    { "TEXCOORD_1", AttributeType::Texcoord1    },
    { "TEXCOORD_2", AttributeType::Texcoord2    },
    { "COLOR_0",    AttributeType::Color0       },
    { "JOINT_0",    AttributeType::Joint0       },
    { "WEIGHTS_0",  AttributeType::Weights0     },
};

}

#endif