vec2 project(vec2 point, float time)
{
    float scale = 1.0 + (sin(time) + 1.0) / 2.0;
    return 2.0 * (point - u_camera) * scale / u_resolution;
}

