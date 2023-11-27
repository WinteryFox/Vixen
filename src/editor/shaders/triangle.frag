#version 450

layout(location = 0) in vec3 vertex_color;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D albedo;

void main() {
    color = texture(albedo, uv) * vec4(vertex_color, 1.0);
}