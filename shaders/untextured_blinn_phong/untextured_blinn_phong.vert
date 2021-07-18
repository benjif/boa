#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inTexCoord;
layout (location = 4) in vec2 inTexCoord1;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec2 outTexCoord;
layout (location = 2) flat out int imageDescriptor;

struct GlobalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
}

layout(set = 0, binding = 0) uniform Transformations {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
} transform;

layout(push_constant) uniform constants {
    ivec4 extra;
    mat4 model_view_projection;
} PushConstants;

void main() {
    gl_Position = PushConstants.model_view_projection * vec4(inPosition, 1.0f);
    outColor = inColor;
    outTexCoord = inTexCoord;
    imageDescriptor = PushConstants.extra.x;
}