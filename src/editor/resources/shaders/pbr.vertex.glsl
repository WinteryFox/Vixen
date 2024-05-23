#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;
layout(location = 3) in vec3 normal;

layout(location = 0) out struct {
    vec4 color;
    vec2 uv;
    vec3 normal;
} o;

layout(push_constant) uniform Instance {
    mat4 transform;
} instance;

layout(binding = 0) uniform Scene {
    mat4 view;
    mat4 projection;
} scene;

void main() {
    gl_Position = scene.projection * scene.view * instance.transform * vec4(position, 1.0);
    o.color = color;
    o.uv = uv;
    o.normal = normal;
}
