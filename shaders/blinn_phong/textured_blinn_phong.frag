#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 inPosition;
layout (location = 3) flat in int imageDescriptor;

layout (location = 0) out vec4 outFragColor;

#define MAX_POINT_LIGHTS 16

struct GlobalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec4 position_and_constant;
    vec4 ambient_and_linear;
    vec4 diffuse_and_quadratic;
    vec4 specular_and_padding;
};

layout (std140, set = 0, binding = 1) uniform BlinnPhong {
    GlobalLight global_light;
    PointLight point_lights[MAX_POINT_LIGHTS];
    uint point_lights_count;
    vec3 camera_position;
} blinn_phong;

layout (set = 1, binding = 0) uniform sampler2D tex[150];

vec3 calculate_global_light(GlobalLight light, vec3 normal, vec3 view_direction, vec3 base_color) {
    vec3 light_direction = normalize(-light.direction);
    vec3 reflect_direction = reflect(-light_direction, normal);
    vec3 half_direction = normalize(light_direction + view_direction);

    float diff = max(dot(normal, light.direction), 0.0);
    float spec;
    if (diff > 0.0f)
        float spec = pow(max(dot(normal, half_direction), 0.0), 8.0f);
    else
        spec = 0.0f;

    vec3 ambient = light.ambient * base_color;
    vec3 diffuse = light.diffuse * diff * base_color;
    vec3 specular = light.specular * spec * base_color;

    return (ambient + diffuse + specular);
}

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 position, vec3 view_direction, vec3 base_color) {
    vec3 light_direction = normalize(light.position_and_constant.xyz - inPosition);
    vec3 reflect_direction = reflect(-light_direction, normal);
    vec3 half_direction = normalize(light_direction + view_direction);

    float diff = max(dot(normal, light_direction), 0.0);
    float spec;
    if (diff > 0.0f)
        float spec = pow(max(dot(normal, half_direction), 0.0), 8.0f);
    else
        spec = 0.0f;

    float distance = length(light.position_and_constant.xyz - inPosition);
    float attenuation = 1.0 / (light.position_and_constant.w + light.ambient_and_linear.w * distance + light.diffuse_and_quadratic.w * (distance * distance));

    vec3 ambient = attenuation * light.ambient_and_linear.xyz * base_color;
    vec3 diffuse = attenuation * light.diffuse_and_quadratic.xyz * diff * base_color;
    vec3 specular = attenuation * light.specular_and_padding.xyz * spec * base_color;

    return (ambient + diffuse + specular);
}

void main() {
    vec3 norm = normalize(inNormal);
    vec3 view_direction = normalize(blinn_phong.camera_position - inPosition);

    vec4 base_color_a = texture(tex[imageDescriptor], texCoord);

    vec3 result = calculate_global_light(blinn_phong.global_light, norm, view_direction, base_color_a.xyz);
    for (uint i = 0; i < blinn_phong.point_lights_count; i++) {
        result += calculate_point_light(blinn_phong.point_lights[i], norm, inPosition, view_direction, base_color_a.xyz);
    }

    outFragColor = vec4(result, base_color_a.a);
}
