#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) flat in int imageDescriptor;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D tex[200];

void main() {
    outFragColor = texture(tex[imageDescriptor], texCoord);
}