#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 outTexCoord;

layout(set = 0, binding = 0) uniform Transformations {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
    mat4 skybox_view_projection;
} transform;

void main() {
    outTexCoord = inPosition;
    vec4 pos = transform.skybox_view_projection * vec4(inPosition, 0.0);
    //gl_Position = pos.xyww;
    gl_Position = pos.xyww;
}