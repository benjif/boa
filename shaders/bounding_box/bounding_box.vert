#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform Transformations {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    mat4 skybox_view_projection;
} transform;

layout(push_constant) uniform constants {
    ivec4 extra0;
    vec4 extra1;
    mat4 model_view_projection;
} PushConstants;

void main() {
    outColor = inColor;
    gl_Position = PushConstants.model_view_projection * vec4(inPosition, 1.0f);
}
