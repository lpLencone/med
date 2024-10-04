#version 330 core

uniform vec2 u_resolution;
uniform vec2 u_pos;
uniform vec2 u_size;
uniform float u_time;

out vec2 v_uv;

vec2 project(vec2 point);

void main() {
    v_uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    gl_Position = vec4(project(v_uv * u_size + u_pos), 0.0, 1.0);
}
