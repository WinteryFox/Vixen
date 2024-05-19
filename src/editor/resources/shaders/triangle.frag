#version 450

layout(location = 0) in vec4 vertex_color;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 color;

layout(binding = 1) uniform sampler2D diffuse;

void main() {
    color = vertex_color * texture(diffuse, uv);
}
