#include "strview.h"

#include <ctype.h>

bool strview_find_first_char(const struct strview s, char c, struct strview *out)
{
    if (!out) {
        return false;
    }

    const unsigned char *ptr = memchr(s.buf, (unsigned char)c, s.size);

    if (!ptr) {
        return false;
    }

    *out = strview_drop(s, (size_t)(ptr - s.buf));

    return true;
}

bool strview_find_first(const struct strview s, const struct strview occurrence, struct strview *out)
{
    if (!out) {
        return false;
    }

    if (occurrence.size == 0) {
        *out = s;
        return true;
    }

    if (s.size < occurrence.size) {
        return false;
    }

    if (occurrence.size == 1) {
        return strview_find_first_char(s, (char)occurrence.buf[0], out);
    }

    for (size_t i = 0; i <= s.size - occurrence.size; ++i) {
        if (memcmp(s.buf + i, occurrence.buf, occurrence.size) == 0) {
            *out = strview_drop(s, i);
            return true;
        }
    }

    return false;
}

struct strview strview_trim_left(const strview s)
{
    uint32_t i = 0;
    while (i < s.size && isspace(s.buf[i])) {
        i += 1;
    }
    return strview_drop(s, i);
}

struct strview strview_trim_right(const strview s)
{
    uint32_t i = 0;
    while (i < s.size && isspace(s.buf[s.size - 1 - i])) {
        i += 1;
    }
    return strview_take(s, s.size - i);
}

struct strview strview_trim(const strview s)
{
    return strview_trim_right(strview_trim_left(s));
}
