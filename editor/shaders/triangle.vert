#version 450

//layout(location = 0) in vec3 position;
vec2 positions[3] = vec2[](
    vec2(0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

void main() {
    //gl_Position = vec4(position, 1.0);
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}
