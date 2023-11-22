#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 vertColor;

layout(binding = 0) uniform ModelViewProjection {
    mat4 model;
    mat4 view;
    mat4 projection;
} mvp;

void main() {
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(position, 1.0);
    vertColor = color;
}