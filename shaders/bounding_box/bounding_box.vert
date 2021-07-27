#version 450

layout (location = 0) in vec3 inPosition;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform constants {
    ivec4 extra0;
    vec4 bounding_box_color;
    mat4 model_view_projection;
} push_constants;

void main() {
    gl_Position = push_constants.model_view_projection * vec4(inPosition, 1.0f);
    outColor = push_constants.bounding_box_color;
}
