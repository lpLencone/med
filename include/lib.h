#ifndef LIB_H_
#define LIB_H_

#define eprint(fmt, ...) fprintf(stderr, "%s::%d::"fmt, __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)
#define eprintln(fmt, ...) eprint(fmt"\n" __VA_OPT__(,) __VA_ARGS__)

#endif // LIB_H_
