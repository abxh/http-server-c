#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct strview {
    size_t size;
    const uint8_t *buf;
} strview;

static const struct strview STRVIEW_EMPTY = {.size = 0, .buf = (const uint8_t *)""};

static inline strview strview_from_cstr(const char *buf)
{
    const size_t size = buf == NULL ? 0 : strlen(buf);
    if (size == 0) {
        return STRVIEW_EMPTY;
    }
    else {
        return (strview){.size = size, .buf = (const uint8_t *)buf};
    }
}

static inline strview strview_from_sized(const uint8_t *buf, const size_t size)
{
    if (buf == NULL || size <= 0) {
        return STRVIEW_EMPTY;
    }
    else {
        return (strview){.size = (size_t)size, .buf = buf};
    }
}

static inline bool strview_equals(const strview lhs, const strview rhs)
{
    if (lhs.size != rhs.size) {
        return false;
    };
    return memcmp(lhs.buf, rhs.buf, lhs.size) == 0;
}

static inline struct strview strview_drop(const strview s, const size_t n)
{
    if (s.size <= n) {
        return STRVIEW_EMPTY;
    }
    else {
        return (struct strview){.size = s.size - n, .buf = s.buf + n};
    }
}

static inline struct strview strview_take(const strview s, const size_t n)
{
    return (struct strview){.size = n <= s.size ? n : s.size, .buf = s.buf};
}

bool strview_find_firstc(const struct strview s, const char c, struct strview *out);

bool strview_find_first(const struct strview s, const struct strview occurrence, struct strview *out);

struct strview strview_trim_left(const strview s);

struct strview strview_trim_right(const strview s);

struct strview strview_trim(const strview s);
