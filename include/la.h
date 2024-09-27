#ifndef LA_H_
#define LA_H_

// 2D vectors

typedef struct { float x, y; } v2f_t;
static inline v2f_t v2f(float x, float y)        { return (v2f_t){ x, y }; }
static inline v2f_t v2fs(float f)                { return (v2f_t){ f, f }; }
#define v2(v) v.x, v.y

static inline v2f_t v2f_neg(v2f_t v)             { return v2f(-v.x, -v.y); }
static inline v2f_t v2f_add(v2f_t a, v2f_t b)    { return v2f(a.x + b.x, a.y + b.y); }
static inline v2f_t v2f_sub(v2f_t a, v2f_t b)    { return v2f(a.x - b.x, a.y - b.y); }
static inline v2f_t v2f_mul(v2f_t a, v2f_t b)    { return v2f(a.x * b.x, a.y * b.y); }
static inline v2f_t v2f_mulf(v2f_t v, float f)   { return v2f(v.x * f, v.y * f); }
static inline v2f_t v2f_div(v2f_t a, v2f_t b)    { return v2f(a.x / b.x, a.y / b.y); }
static inline v2f_t v2f_divf(v2f_t v, float f)   { return v2f(v.x / f, v.y / f); }

typedef struct { int x, y; } v2i_t;
static inline v2i_t v2i(int x, int y)           { return (v2i_t){ x, y }; }
static inline v2i_t v2is(int i)                 { return (v2i_t){ i, i }; }

static inline v2i_t v2i_neg(v2i_t v)            { return v2i(-v.x, -v.y); }
static inline v2i_t v2i_add(v2i_t a, v2i_t b)   { return v2i(a.x + b.x, a.y + b.y); }
static inline v2i_t v2i_sub(v2i_t a, v2i_t b)   { return v2i(a.x - b.x, a.y - b.y); }
static inline v2i_t v2i_mul(v2i_t a, v2i_t b)   { return v2i(a.x * b.x, a.y * b.y); }
static inline v2i_t v2i_mulf(v2i_t v, int i)    { return v2i(v.x * i, v.y * i); }
static inline v2i_t v2i_div(v2i_t a, v2i_t b)   { return v2i(a.x / b.x, a.y / b.y); }
static inline v2i_t v2i_divf(v2i_t v, int i)    { return v2i(v.x / i, v.y / i); }

// 3D vectors

typedef struct { float x, y, z; } v3f_t;
static inline v3f_t v3f(float x, float y, float z)  { return (v3f_t){ x, y, z }; }
static inline v3f_t v3fs(float f)                   { return (v3f_t){ f, f, f }; }
#define v3(v) v.x, v.y, v.z

static inline v3f_t v3f_add   (v3f_t a, v3f_t b)     { return (v3f_t){ a.x + b.x, a.y + b.y, a.z + b.z }; }
static inline v3f_t v3f_adds  (v3f_t a, float f)     { return (v3f_t){ a.x + f,   a.y + f,   a.z + f   }; }
static inline v3f_t v3f_sub   (v3f_t a, v3f_t b)     { return (v3f_t){ a.x - b.x, a.y - b.y, a.z - b.z }; }
static inline v3f_t v3f_subs  (v3f_t a, float f)     { return (v3f_t){ a.x - f,   a.y - f,   a.z - f   }; }
static inline v3f_t v3f_mul   (v3f_t a, v3f_t b)     { return (v3f_t){ a.x * b.x, a.y * b.y, a.z * b.z }; }
static inline v3f_t v3f_mulf  (v3f_t a, float f)     { return (v3f_t){ a.x * f,   a.y * f,   a.z * f   }; }
static inline v3f_t v3f_div   (v3f_t a, v3f_t b)     { return (v3f_t){ a.x / b.x, a.y / b.y, a.z / b.z }; }
static inline v3f_t v3f_divf  (v3f_t a, float f)     { return (v3f_t){ a.x / f,   a.y / f,   a.z / f   }; }

// 4D Vectors

typedef struct { float x, y, z, w; } v4f_t;
static inline v4f_t v4f(float x, float y, float z, float w)   { return (v4f_t){ x, y, z, w }; }
static inline v4f_t v4fs(float f)                             { return (v4f_t){ f, f, f, f }; }
#define v4(v) v.x, v.y, v.z, v.w

static inline v4f_t v4f_neg(v4f_t v) { return v4f(-v.x, -v.y, -v.z, -v.w); }
static inline v4f_t v4f_add(v4f_t a, v4f_t b) { return v4f(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w); }
// static inline v4f_t v4f_sub(v4f_t a, v4f_t b) { return v4f(a.x - b.x, a.y - b.y); }
// static inline v4f_t v4f_mul(v4f_t a, v4f_t b) { return v4f(a.x * b.x, a.y * b.y); }
// static inline v4f_t v4f_mulf(v4f_t v, float f) { return v4f(v.x * f, v.y * f); }
// static inline v4f_t v4f_div(v4f_t a, v4f_t b) { return v4f(a.x / b.x, a.y / b.y); }
// static inline v4f_t v4f_divf(v4f_t v, float f) { return v4f(v.x / f, v.y / f); }

#endif // LA_H_
