// inspiration:
// github.com/tsoding/sv

#include "strview.h"

#include <ctype.h>

bool strview_find_firstc(const struct strview s, const uint8_t c, struct strview *out)
{
    if (!out) {
        return false;
    }

    const uint8_t *ptr = memchr(s.buf, c, s.length);
    if (!ptr) {
        return false;
    }

    *out = strview_drop(s, (size_t)(ptr - s.buf));

    return true;
}

bool strview_find_first(const struct strview s, const struct strview occurrence, struct strview *out)
{
    if (!out || s.length < occurrence.length) {
        return false;
    }

    switch (occurrence.length) {
    case 0:
        *out = s;
        return true;

    case 1:
        return strview_find_firstc(s, occurrence.buf[0], out);

    default:
        for (size_t i = 0; i <= s.length - occurrence.length; i++) {
            if (memcmp(s.buf + i, occurrence.buf, occurrence.length) == 0) {
                *out = strview_drop(s, i);
                return true;
            }
        }
        return false;
    };
}

bool strview_find_lastc(const struct strview s, const uint8_t c, struct strview *out)
{
    if (!out) {
        return false;
    }

    for (size_t i = s.length; i > 0; i--) {
        if (s.buf[i - 1] == c) {
            *out = strview_drop(s, i - 1);
            return true;
        }
    }

    return false;
}

bool strview_find_last(const struct strview s, const struct strview occurrence, struct strview *out)
{
    if (!out || s.length < occurrence.length) {
        return false;
    }

    switch (occurrence.length) {
    case 0:
        *out = s;
        return true;

    case 1:
        return strview_find_lastc(s, occurrence.buf[0], out);

    default:
        for (size_t i = s.length - occurrence.length + 1; i > 0; i--) {
            const size_t pos = i - 1;
            if (memcmp(s.buf + pos, occurrence.buf, occurrence.length) == 0) {
                *out = strview_drop(s, pos);
                return true;
            }
        }
        return false;
    }
}

struct strview strview_trim_left(const strview s)
{
    size_t i = 0;
    while (i < s.length && isspace(s.buf[i])) {
        i += 1;
    }
    return strview_drop(s, i);
}

struct strview strview_trim_right(const strview s)
{
    size_t i = 0;
    while (i < s.length && isspace(s.buf[s.length - 1 - i])) {
        i += 1;
    }
    return strview_take(s, s.length - i);
}

struct strview strview_trim(const strview s)
{
    return strview_trim_right(strview_trim_left(s));
}
