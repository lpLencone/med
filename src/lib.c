#include "lib.h"

slice_t slice_from(void const *data, size_t length)
{
    return (slice_t) {
        .data = data,
        .length = length,
    };
}
