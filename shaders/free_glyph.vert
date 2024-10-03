#version 330 core

#define FONT_SCALE 4.0

layout(location = 0) in vec2 l_pos;
layout(location = 1) in vec2 l_size;
layout(location = 2) in vec2 l_uv_pos;
layout(location = 3) in vec2 l_uv_size;
layout(location = 4) in vec4 l_fg;
layout(location = 5) in vec4 l_bg;

uniform vec2 u_resolution;
uniform float u_time;
uniform vec2 u_camera;

out vec4 v_fg;
out vec4 v_bg;
out vec2 v_uv;
out vec2 v_uv_pos;
out vec2 v_uv_size;

#use "shaders/project.glsl"

void main() {
    v_uv_pos = l_uv_pos;
    v_uv_size = l_uv_size;
    v_fg = l_fg;
    v_bg = l_bg;
    v_uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));

    gl_Position = vec4(project(v_uv * l_size + l_pos, u_time), 0.0, 1.0);
}
