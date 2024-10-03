#version 330 core

uniform vec2 u_resolution;
uniform vec2 u_camera;
uniform vec2 u_pos;
uniform vec2 u_size;
uniform float u_time;

out vec2 v_uv;

#use "shaders/project.glsl"

void main() {
    v_uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    gl_Position = vec4(project(v_uv * u_size + u_pos, u_time), 0.0, 1.0);
}
