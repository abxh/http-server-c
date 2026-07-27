#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct strview {
    size_t size;     // string size without null character
    const char *buf; // the underlying buffer, possibly with null character
} strview;

static inline strview strview_from(const char *buf)
{
    return (strview){.size = strlen(buf), .buf = buf};
}

static inline strview strview_from_sized(const char *buf, ptrdiff_t size)
{
    return (strview){.size = strnlen(buf, (size_t)(size > 0 ? size : 0)), .buf = buf};
}

bool strview_equals(const strview lhs, const strview rhs);
