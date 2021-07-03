#version 450
//#extension GL_ARB_separate_shader_objects : enable

//layout(location = 0) in vec2 inPosition;
//layout(location = 1) in vec3 inColor;

//layout(location = 0) out vec3 fragColor;

void main() {
    const vec3 positions[3] = vec3[3](
        vec3(1.f, 1.f, 0.0f),
        vec3(-1.f, 1.f, 0.0f),
        vec3(0.f, -1.f, 0.0f)
    );

    gl_Position = vec4(positions[gl_VertexIndex], 1.0f);

    //gl_Position = vec4(inPosition, 0.0, 1.0);
    //fragColor = inColor;
}