uniform float u_scale;
uniform vec2 u_camera;
uniform vec2 u_resolution;

vec2 project(vec2 point)
{
    return 2.0 * (point - u_camera) * u_scale / u_resolution;
}

