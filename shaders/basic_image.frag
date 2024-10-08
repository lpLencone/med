#version 330 core

uniform sampler2D image;

in vec2 p_uv;
in vec4 p_color;

void main() {
    gl_FragColor = p_color * texture(image, p_uv).x;
}
