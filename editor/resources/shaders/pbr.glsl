#version 450

layout (location = 0) in vec3 position;
layout (location = 1) in vec4 color;
layout (location = 2) in vec2 uv;
layout (location = 3) in vec3 normal;

layout (location = 0) out struct {
    vec4 color;
    vec2 uv;
    vec3 normal;
} o;

layout (push_constant) uniform Instance {
    mat4 model;
} instance;

layout (binding = 0) uniform Scene {
    mat4 view;
    mat4 projection;
} scene;

void vertex() {
    gl_Position = scene.projection * scene.view * instance.model * vec4(position, 1.0);
    o.color = color;
    o.uv = uv;
    // TODO: Calculate the normal matrix outside of the shader and pass it in
    o.normal = mat3(transpose(inverse(instance.model))) * normal;
}

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float ambientStrength;
};

layout (location = 0) in struct {
    vec4 color;
    vec2 uv;
    vec3 normal;
} i;

layout (location = 0) out vec4 color;

layout (binding = 1) uniform sampler2D diffuseTexture;

const vec3 direction = vec3(1.0, -3.0, -1.0);

vec3 calculateDirectionalLight(DirectionalLight light) {
    float ambient = light.ambientStrength;
    float diffuse = max(dot(normalize(-light.direction), normalize(i.normal)), 0.0);

    return (ambient + diffuse) * texture(diffuseTexture, i.uv).rgb;
}

void fragment() {
    color = vec4(calculateDirectionalLight(DirectionalLight(direction, vec3(0.25, 0.7, 0.4), 0.1f)), 1.0f);
}
