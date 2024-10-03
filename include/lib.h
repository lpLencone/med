#ifndef LIB_H_
#define LIB_H_

#include <stddef.h>

#define eprint(fmt, ...) \
    fprintf(stderr, "%s::%d::" fmt, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define eprintln(fmt, ...) eprint(fmt "\n" __VA_OPT__(, ) __VA_ARGS__)

#define panic(fmt, ...)                           \
    do {                                          \
        eprintln(fmt __VA_OPT__(, ) __VA_ARGS__); \
        exit(1);                                  \
    } while (0)

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif // max

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif // min

typedef struct {
    void const *data;
    size_t length;
} slice_t;

slice_t slice_from(void const *data, size_t length);

#endif // LIB_H_
