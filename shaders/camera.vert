#version 330 core

layout(location = 0) in vec2 l_uv;
layout(location = 1) in vec2 l_pos;
layout(location = 2) in vec4 l_color;

out vec2 p_uv;
out vec4 p_color;

vec2 project(vec2 point);

void main() {
    p_uv = l_uv;
    p_color = l_color;
    gl_Position = vec4(project(l_pos), 0.0, 1.0);
}
