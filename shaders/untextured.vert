#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inColor;

layout (location = 0) out vec3 outColor;
//layout (location = 1) out vec2 outTexCoord;

layout(set = 0, binding = 0) uniform Transformations {
    mat4 view;
    mat4 projection;
    mat4 view_projection;
} transform;

layout(push_constant) uniform constants {
    vec4 extra;
    mat4 model_view_projection;
} PushConstants;

void main() {
    gl_Position = PushConstants.model_view_projection * vec4(inPosition, 1.0f);
    outColor = inNormal;
    //outTexCoord = inTexCoord;
}