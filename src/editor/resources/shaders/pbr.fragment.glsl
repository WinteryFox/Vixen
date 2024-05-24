#version 450

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float ambientStrength;
};

layout(location = 0) in struct {
    vec4 color;
    vec2 uv;
    vec3 normal;
} i;

layout(location = 0) out vec4 color;

layout(binding = 1) uniform sampler2D diffuseTexture;

const vec3 direction = vec3(1.0, -3.0, -1.0);

vec3 calculateDirectionalLight(DirectionalLight light) {
    float ambient = light.ambientStrength;
    float diffuse = max(dot(normalize(-light.direction), normalize(i.normal)), 0.0);

    return (ambient + diffuse) * texture(diffuseTexture, i.uv).rgb;
}

void main() {
    color = vec4(calculateDirectionalLight(DirectionalLight(direction, vec3(0.25, 0.7, 0.4), 0.1f)), 1.0f);
}
