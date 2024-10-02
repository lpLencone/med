#version 330 core

#define BLINK_THRESHOLD 0.5
#define PERIOD 0.5
#define M_PI 3.14159

uniform float u_time;
uniform float u_last_moved;

float map01(float f) {
    return (1.0 + f) / 2.0;
}

void main() {
    float time = u_time - u_last_moved;
    float shine = float(time < BLINK_THRESHOLD);
    float wave = map01(cos(M_PI * (time - BLINK_THRESHOLD) / PERIOD));
    float opacity = max(shine, wave);
    gl_FragColor = vec4(1.0, 1.0, 1.0, opacity);

    // square wave
    // gl_FragColor = vec4(1.0) * (1 - (int(time / PERIOD) % 2));
}
