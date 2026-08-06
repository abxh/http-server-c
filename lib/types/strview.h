#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct strview {
    size_t length;
    const uint8_t *buf;
} strview_t;

static const strview_t STRVIEW_EMPTY = {.length = 0, .buf = (const uint8_t *)""};

#define STRVIEW(cstr)      {.length = sizeof(cstr) - 1, .buf = (const uint8_t *)cstr}
#define STRVIEW_FROM(cstr) (strview_t) STRVIEW(cstr)

static inline strview_t strview_from_cstr(const char *buf)
{
    const size_t size = buf == NULL ? 0 : strlen(buf);
    if (size == 0) {
        return STRVIEW_EMPTY;
    }
    else {
        return (strview_t){.length = size, .buf = (const uint8_t *)buf};
    }
}

static inline strview_t strview_from_sized(const uint8_t *buf, const size_t length)
{
    if (buf == NULL) {
        return STRVIEW_EMPTY;
    }
    else {
        return (strview_t){.length = length, .buf = buf};
    }
}

static inline bool strview_equals(const strview_t lhs, const strview_t rhs)
{
    if (lhs.length != rhs.length) {
        return false;
    };
    return memcmp(lhs.buf, rhs.buf, lhs.length) == 0;
}

static inline strview_t strview_drop(const strview_t s, const size_t n)
{
    if (s.length <= n) {
        return STRVIEW_EMPTY;
    }
    else {
        return (strview_t){.length = s.length - n, .buf = s.buf + n};
    }
}

static inline strview_t strview_take(const strview_t s, const size_t n)
{
    return (strview_t){.length = n <= s.length ? n : s.length, .buf = s.buf};
}

bool strview_find_firstc(const strview_t s, const uint8_t c, strview_t *out);
bool strview_find_first(const strview_t s, const strview_t occurrence, strview_t *out);
bool strview_find_lastc(const strview_t s, const uint8_t c, strview_t *out);
bool strview_find_last(const strview_t s, const strview_t occurrence, strview_t *out);

strview_t strview_trim_left(const strview_t s);
strview_t strview_trim_right(const strview_t s);
strview_t strview_trim(const strview_t s);
