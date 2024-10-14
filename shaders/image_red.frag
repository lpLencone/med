#version 330 core

uniform sampler2D image;

in vec2 p_uv;
in vec4 p_color;

void main() {
    float tx = texture(image, p_uv).r;
    float aaf = fwidth(tx);
    float alpha = smoothstep(0.5 - aaf, 0.5 + aaf, tx);
    gl_FragColor = vec4(p_color.rgb, alpha);
}
