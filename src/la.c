#include "la.h"

v2f_t v2f(float x, float y)
{
    return (v2f_t) { .x = x, .y = y };
}

v2f_t v2fs(float f)
{
    return v2f(f, f);
}

v2f_t v2f_add(v2f_t a, v2f_t b)
{
    return v2f(a.x + b.x, a.y + b.y);
}

v2f_t v2f_sub(v2f_t a, v2f_t b)
{
    return v2f(a.x - b.x, a.y - b.y);
}

v2f_t v2f_mul(v2f_t a, v2f_t b)
{
    return v2f(a.x * b.x, a.y * b.y);
}

v2f_t v2f_mulf(v2f_t v, float f)
{
    return v2f(v.x * f, v.y * f);
}

v2f_t v2f_div(v2f_t a, v2f_t b)
{
    return v2f(a.x / b.x, a.y / b.y);
}

v2f_t v2f_divf(v2f_t v, float f)
{
    return v2f(v.x / f, v.y / f);
}
