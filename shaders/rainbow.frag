#version 330 core

uniform sampler2D image;
uniform float u_time;
uniform vec2 u_resolution;

in vec2 p_uv;
in vec4 p_color;

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

void main() {
    float tx = texture(image, p_uv).x;
    float aaf = fwidth(tx);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, tx);

    vec2 frag_uv = gl_FragCoord.xy / u_resolution;
    vec4 rainbow = vec4(hsl2rgb(vec3(frag_uv.x + frag_uv.y + u_time, 0.5, 0.5)), alpha);

    gl_FragColor = p_color * rainbow;
}
