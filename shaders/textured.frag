#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D tex;

void main() {
    //outFragColor = vec4(inColor, 1.0f);
    //outFragColor = vec4(texCoord.x, texCoord.y, 0.5f, 1.0f);
    outFragColor = vec4(texture(tex, texCoord).xyz, 1.0f);
    //outFragColor = texture(tex, texCoord);
}