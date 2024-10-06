#version 330 core

uniform sampler2D image;

in vec2 p_uv;

void main() {
    gl_FragColor = texture(image, p_uv);
}
