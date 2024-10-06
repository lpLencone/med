#version 330 core

layout(location = 0) in vec2 uv;
layout(location = 1) in vec2 pos;
layout(location = 2) in vec4 color;

out vec2 p_uv;
out vec4 p_color;

vec2 project(vec2 point);

void main() {
    p_uv = uv;
    p_color = color;
    gl_Position = vec4(project(pos), 0.0, 1.0);
}
