#version 450

layout(location = 0) in struct {
    vec4 color;
    vec2 uv;
    vec3 normal;
} i;

layout(location = 0) out vec4 color;

layout(binding = 1) uniform sampler2D diffuse;

void main() {
    color = i.color * texture(diffuse, i.uv);
}
