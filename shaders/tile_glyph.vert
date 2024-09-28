#version 330 core

#define FONT_WIDTH       128.0
#define FONT_HEIGHT      64.0
#define FONT_ROWS        7
#define FONT_COLS        18
#define FONT_CHAR_WIDTH  (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE       4.0

layout(location = 0) in ivec2 l_tile;
layout(location = 1) in int l_ch;
layout(location = 2) in vec4 l_fg;
layout(location = 3) in vec4 l_bg;

uniform vec2 u_resolution;
uniform float u_scale;
uniform float u_time;
uniform vec2 u_camera;

out vec2 uv;
out vec4 v_fg;
out vec4 v_bg;
flat out int v_ch;

vec2 project_point(vec2 point)
{
    return 2.0 * (point - u_camera) / u_resolution;
}

void main() {
    v_ch = l_ch;
    v_fg = l_fg;
    v_bg = l_bg;

    uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    vec2 shaking = vec2(cos(u_time +  l_tile.y), sin(u_time + l_tile.x));
    vec2 tile = l_tile;

    vec2 char_size = vec2(FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT);
    gl_Position = vec4(project_point((uv + tile) * u_scale * char_size), 0.0, 1.0);
}
