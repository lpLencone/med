#version 330 core

layout(location = 0) in vec2 l_uv;
layout(location = 1) in vec2 l_pos;
layout(location = 2) in vec4 l_color;

out vec2 p_uv;
out vec4 p_color;

uniform float u_scale;
uniform vec2 u_camera;
uniform vec2 u_resolution;

vec2 project(vec2 point)
{
    return 2.0 * (point - u_camera) * u_scale / u_resolution;
}

void main() {
    p_uv = l_uv;
    p_color = l_color;
    gl_Position = vec4(project(l_pos), 0.0, 1.0);
}
