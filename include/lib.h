#ifndef LIB_H_
#define LIB_H_

#define eprint(fmt, ...) \
    fprintf(stderr, "%s::%d::" fmt, __FILE__, __LINE__ __VA_OPT__(, ) __VA_ARGS__)
#define eprintln(fmt, ...) eprint(fmt "\n" __VA_OPT__(, ) __VA_ARGS__)

#define panic(fmt, ...)                           \
    do {                                          \
        eprintln(fmt __VA_OPT__(, ) __VA_ARGS__); \
        exit(1);                                  \
    } while (0)

#endif // LIB_H_
