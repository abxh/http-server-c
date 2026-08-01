#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct strview {
    size_t length;
    const uint8_t *buf;
} strview;

static const struct strview STRVIEW_EMPTY = {.length = 0, .buf = (const uint8_t *)""};

#define STRVIEW(cstr)      {.length = sizeof(cstr) - 1, .buf = (const uint8_t *)cstr}
#define STRVIEW_FROM(cstr) (struct strview) STRVIEW(cstr)

static inline strview strview_from_cstr(const char *buf)
{
    const size_t size = buf == NULL ? 0 : strlen(buf);
    if (size == 0) {
        return STRVIEW_EMPTY;
    }
    else {
        return (strview){.length = size, .buf = (const uint8_t *)buf};
    }
}

static inline strview strview_from_sized(const uint8_t *buf, const size_t length)
{
    if (buf == NULL) {
        return STRVIEW_EMPTY;
    }
    else {
        return (strview){.length = length, .buf = buf};
    }
}

static inline bool strview_equals(const strview lhs, const strview rhs)
{
    if (lhs.length != rhs.length) {
        return false;
    };
    return memcmp(lhs.buf, rhs.buf, lhs.length) == 0;
}

static inline struct strview strview_drop(const strview s, const size_t n)
{
    if (s.length <= n) {
        return STRVIEW_EMPTY;
    }
    else {
        return (struct strview){.length = s.length - n, .buf = s.buf + n};
    }
}

static inline struct strview strview_take(const strview s, const size_t n)
{
    return (struct strview){.length = n <= s.length ? n : s.length, .buf = s.buf};
}

bool strview_find_firstc(const struct strview s, const uint8_t c, struct strview *out);
bool strview_find_first(const struct strview s, const struct strview occurrence, struct strview *out);
bool strview_find_lastc(const struct strview s, const uint8_t c, struct strview *out);
bool strview_find_last(const struct strview s, const struct strview occurrence, struct strview *out);

struct strview strview_trim_left(const strview s);
struct strview strview_trim_right(const strview s);
struct strview strview_trim(const strview s);
