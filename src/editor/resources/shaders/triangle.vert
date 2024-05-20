#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv;

layout(location = 0) out struct {
    vec4 color;
    vec2 uv;
} o;

layout(binding = 0) uniform ModelViewProjection {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

void main() {
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(position, 1.0);
    o.color = color;
    o.uv = uv;
}
