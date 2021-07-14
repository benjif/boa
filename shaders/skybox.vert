#version 450

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec3 outTexCoord;

layout(set = 0, binding = 0) uniform Transformations {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
} transform;

void main() {
    outTexCoord = inPosition;
    gl_Position = transform.view_projection * vec4(inPosition, 1.0);
}