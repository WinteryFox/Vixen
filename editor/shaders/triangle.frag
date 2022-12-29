#version 330 core
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 color;

void main() {
    color = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
