#version 330 core

#define FONT_WIDTH          128
#define FONT_HEIGHT         64
#define FONT_ROWS           7
#define FONT_COLS           18
#define FONT_CHAR_WIDTH     (FONT_WIDTH / FONT_COLS)
#define FONT_CHAR_HEIGHT    (FONT_HEIGHT / FONT_ROWS)
#define FONT_CHAR_WIDTH_UV  (float(FONT_CHAR_WIDTH) / float(FONT_WIDTH))
#define FONT_CHAR_HEIGHT_UV (float(FONT_CHAR_HEIGHT) / float(FONT_HEIGHT))

uniform sampler2D font;
uniform float time;

in vec2 uv;
in vec4 v_fg;
in vec4 v_bg;
flat in int v_ch;

float map01(float f) {
    return (1.0 + f) / 2.0;
}

void main() {
    int ch = v_ch;
    if (ch < 32 || ch > 126) {
        ch = 63;
    }

    int index = ch - 32;
    float col = index % FONT_COLS * FONT_CHAR_WIDTH_UV;
    float row = index / FONT_COLS * FONT_CHAR_HEIGHT_UV;

    vec2 pos = vec2(col, row + FONT_CHAR_HEIGHT_UV);
    vec2 size = vec2(FONT_CHAR_WIDTH_UV, -FONT_CHAR_HEIGHT_UV);
    vec2 t = pos + size * uv;

    vec4 tc = texture(font, t);
    vec4 rainbow = vec4(
            map01(sin(time + uv.x)),
            map01(cos(time + uv.y)),
            map01(sin(time + uv.y) * cos(time + uv.x)),
            1.0);

    gl_FragColor = v_bg * (1 - tc.x) + tc.x * v_fg * rainbow;
}
