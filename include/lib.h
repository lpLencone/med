#ifndef LIB_H_
#define LIB_H_

#define eprint(fmt, ...) fprintf(stderr, fmt __VA_OPT__(,) __VA_ARGS__)
#define eprintln(fmt, ...) eprint(fmt"\n" __VA_OPT__(,) __VA_ARGS__)

#endif // LIB_H_
