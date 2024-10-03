vec2 project_point(vec2 point)
{
    return 2.0 * (point - u_camera) / u_resolution;
}

