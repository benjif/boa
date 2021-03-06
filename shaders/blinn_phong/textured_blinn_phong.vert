#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outTexCoord;
layout (location = 2) out vec3 outPosition;
layout (location = 3) flat out int imageDescriptor;

#define COLOR_BASE_COLOR 0
#define COLOR_VERT_COLOR 1

layout(set = 0, binding = 0) uniform Transformations {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
} transform;

layout(push_constant) uniform constants {
    ivec4 image_descriptor_and_color_type;
    vec4 base_color;
    mat4 model;
} push_constants;

void main() {
    gl_Position = transform.view_projection * push_constants.model * vec4(inPosition, 1.0f);

    //outNormal = mat3(transpose(inverse(push_constants.model))) * inNormal;
    outNormal = inNormal;
    outPosition = vec3(push_constants.model * vec4(inPosition, 1.0));
    outTexCoord = inTexCoord;

    imageDescriptor = push_constants.image_descriptor_and_color_type.x;
}
