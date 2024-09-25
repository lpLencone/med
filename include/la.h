#ifndef LA_H_
#define LA_H_

typedef struct {
    float x, y;
} v2f_t;

v2f_t v2f(float x, float y);
v2f_t v2fs(float f);
v2f_t v2f_add(v2f_t a, v2f_t b);
v2f_t v2f_sub(v2f_t a, v2f_t b);
v2f_t v2f_mul(v2f_t a, v2f_t b);
v2f_t v2f_mulf(v2f_t v, float f);
v2f_t v2f_div(v2f_t a, v2f_t b);
v2f_t v2f_divf(v2f_t v, float f);

#endif // LA_H_
