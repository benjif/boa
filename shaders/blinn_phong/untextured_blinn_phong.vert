#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outPosition;

#define COLOR_BASE_COLOR 0
#define COLOR_VERT_COLOR 1

layout(set = 0, binding = 0) uniform Transformations {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
} transform;

layout(push_constant) uniform constants {
    ivec4 padding_and_color_type;
    vec4 base_color;
    mat4 model;
    mat4 model_view_projection;
} push_constants;

void main() {
    gl_Position = push_constants.model_view_projection * vec4(inPosition, 1.0f);
    //outNormal = mat3(transpose(inverse(push_constants.model))) * inNormal;
    outNormal = inNormal;
    outPosition = vec3(push_constants.model * vec4(inPosition, 1.0));

    switch (push_constants.padding_and_color_type.y) {
    case COLOR_BASE_COLOR:
        outColor = push_constants.base_color;
        break;
    case COLOR_VERT_COLOR:
    default:
        outColor = inColor;
        break;
    }

    outColor = inColor;
}
