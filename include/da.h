#ifndef DA_H_
#define DA_H_

#ifndef DA_INIT_CAP
    #define DA_INIT_CAP 4
#endif // DA_INIT_CAP

#ifndef DA_GROW_RATE
    #define DA_GROW_RATE 2.0
#endif // DA_GROW_RATE

#define DA_TYPESIZE(a) (sizeof *(a)->data)

#define da(type_t)       \
    struct {             \
        type_t *data;    \
        size_t length;   \
        size_t capacity; \
    }

#define da_free(da)                            \
    do {                                       \
        if ((da)->capacity > 0) {              \
            free((da)->data);                  \
            (da)->length = (da)->capacity = 0; \
        }                                      \
    } while (0)

#define da_grow_n(a, n)                                                     \
    do {                                                                    \
        size_t cap = (a)->capacity;                                         \
        if (cap < DA_INIT_CAP) {                                            \
            cap = DA_INIT_CAP;                                              \
        }                                                                   \
        while (cap < (a)->length + n) {                                     \
            cap *= DA_GROW_RATE;                                            \
        }                                                                   \
        if (cap != (a)->capacity) {                                         \
            (a)->capacity = cap;                                            \
            (a)->data = realloc((a)->data, DA_TYPESIZE(a) * (a)->capacity); \
        }                                                                   \
    } while (false)

#define da_insert_n(a, src, n, index)                       \
    do {                                                    \
        da_grow_n(a, n);                                    \
        memmove((a)->data + index + n, (a)->data + index,   \
                ((a)->length - index) * DA_TYPESIZE(a));    \
        memcpy((a)->data + index, src, DA_TYPESIZE(a) * n); \
        (a)->length += (n);                                 \
    } while (false)

#define da_push_n(a, src, n) da_insert_n(a, src, n, (a)->length)
#define da_push(a, src)      da_push_n(a, src, 1)

#define da_shrink(a)                                                             \
    do {                                                                         \
        if (2 * (a)->length < (a)->capacity && 2 * DA_INIT_CAP <= (a)->length) { \
            (a)->capacity /= 2;                                                  \
            (a)->data = realloc((a)->data, DA_TYPESIZE(a) * (a)->capacity);      \
        }                                                                        \
    } while (false)

#define da_remove_n(a, n, index)                             \
    do {                                                     \
        assert(index + n <= (a)->length);                    \
        memmove((a)->data + index, (a)->data + index + n,    \
                ((a)->length - index + n) * DA_TYPESIZE(a)); \
        (a)->length -= (n);                                  \
        da_shrink(a);                                        \
    } while (false)

#define da_remove(a, index) da_remove_n(a, 1, index)

#endif // DA_H_
