#version 330 core

#define FONT_WIDTH       128.0
#define FONT_HEIGHT      64.0
#define FONT_ROWS        7
#define FONT_COLS        18
#define FONT_CHAR_WIDTH  (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT (FONT_HEIGHT / FONT_ROWS)
#define FONT_SCALE       4.0

layout (location = 0) in vec2 l_pos;
layout (location = 1) in float l_scale;
layout (location = 2) in float l_ch;
layout (location = 3) in vec4 l_color;

uniform vec2 resolution;

out vec2 uv;
out vec4 v_color;
out float v_ch;

vec2 project_point(vec2 point)
{
    return 2.0 * point / resolution;
}

void main() {
    v_ch = l_ch;
    v_color = l_color;
    uv = vec2(float(gl_VertexID & 1), float((gl_VertexID >> 1) & 1));
    vec2 char_size = vec2(FONT_CHAR_WIDTH, FONT_CHAR_HEIGHT);
    gl_Position = vec4(project_point(uv * char_size * l_scale + l_pos), 0.0, 1.0);
}
