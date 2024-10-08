#version 330 core

uniform sampler2D font;
uniform float u_time;
uniform vec2 u_resolution;

in vec4 v_fg;
in vec2 v_uv;
in vec2 v_uv_pos;
in vec2 v_uv_size;

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

float map01(float f) {
    return (1.0 + f) / 2.0;
}

void main() {
    vec2 t = v_uv_pos + v_uv_size * v_uv;
    vec4 tc = texture(font, t);
    vec2 frag_uv = gl_FragCoord.xy / u_resolution;
    vec4 rainbow = vec4(hsl2rgb(vec3(frag_uv.x + frag_uv.y + u_time, 0.5, 0.5)), 1.0);

    gl_FragColor = tc.x * v_fg * rainbow;
}
