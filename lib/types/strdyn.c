// inspiration:
// https://github.com/antirez/sds

#include "strdyn.h"

#include "../error.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct strdyn_impl {
    size_t length;
    size_t capacity;
    char buf[];
} strdyn_impl_t;

#define INITIAL_SIZE  (8)
#define BUFFER_OFFSET (offsetof(struct strdyn_impl, buf))

static inline strdyn_impl_t *container_of_strdyn(strdyn_t s)
{
    return (strdyn_impl_t *)(s - BUFFER_OFFSET);
}

static inline const strdyn_impl_t *container_of_strdyn_const(const strdyn_t s)
{
    return (const strdyn_impl_t *)(s - BUFFER_OFFSET);
}

void strdyn_free(strdyn_t s)
{
    if (s != NULL) free(s - BUFFER_OFFSET);
}

size_t strdyn_length(const strdyn_t s)
{
    if (s == NULL) return 0;
    const strdyn_impl_t *c = container_of_strdyn_const(s);
    return c->length;
}

void strdyn_clear(strdyn_t s)
{
    assert(s != NULL);
    strdyn_impl_t *c = container_of_strdyn(s);
    assert(c->capacity != 0);
    c->length = 0;
    c->buf[0] = '\0';
}

Error_t strdyn_reserve_(const ErrorInfo_t ei, strdyn_t *out, const size_t len)
{
    RETURN_IF_NULL(ei, out);
    if (len > ((SIZE_MAX - 1) / sizeof(char)) - BUFFER_OFFSET) {
        return error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "allocated size overflows"});
    }
    if (*out == NULL) {
        strdyn_impl_t *s = calloc(1, BUFFER_OFFSET + (len + 1) * sizeof(char));
        if (!s) {
            return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
        }
        s->length = 0;
        s->capacity = len + 1;
        s->buf[0] = '\0';
        *out = (char *)s + BUFFER_OFFSET;
        return NO_ERRORS;
    }
    else {
        strdyn_impl_t *prev = container_of_strdyn(*out);
        const size_t prev_len = prev->length;

        strdyn_impl_t *next = realloc(prev, BUFFER_OFFSET + (len + 1) * sizeof(char));
        if (!next) {
            return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
        }

        next->length = prev_len;
        next->capacity = len + 1;
        next->buf[prev_len] = '\0';

        *out = (char *)next + BUFFER_OFFSET;
        return NO_ERRORS;
    }
}

Error_t strdyn_empty_(const ErrorInfo_t ei, strdyn_t *out)
{
    *out = NULL;
    return strdyn_reserve_(ei, out, INITIAL_SIZE);
}

static Error_t strdyn_update_capacity(const ErrorInfo_t ei, strdyn_t *out, const size_t min_capacity)
{
    strdyn_impl_t *c = container_of_strdyn(*out);
    if (c->capacity >= min_capacity) {
        return NO_ERRORS;
    }
    if (min_capacity < INITIAL_SIZE) {
        return strdyn_reserve_(ei, out, INITIAL_SIZE);
    }
    if (min_capacity > (SIZE_MAX / 3) * 2) {
        return error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "new capacity overflows"});
    }
    return strdyn_reserve_(ei, out, min_capacity * 3 / 2);
}

Error_t strdyn_append_len_(const ErrorInfo_t ei, strdyn_t *out, const char *suffix, const size_t suffix_len)
{
    RETURN_IF_NULL(ei, out);
    RETURN_IF_NULL(ei, *out);

    strdyn_impl_t *c = container_of_strdyn(*out);
    if (c->length > SIZE_MAX - suffix_len) {
        return error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "sum of sizes overflows"});
    }
    const size_t new_len = c->length + suffix_len;

    const Error_t error = strdyn_update_capacity(ei, out, new_len);
    if (error.tag != ERROR_NONE) {
        return error;
    }
    c = container_of_strdyn(*out);

    memmove(*out + c->length, suffix, suffix_len);
    c->length = new_len;
    c->buf[c->length] = '\0';

    return NO_ERRORS;
}



Error_t strdyn_append_(const ErrorInfo_t ei, strdyn_t *out, const char *suffix)
{
    return strdyn_append_len_(ei, out, suffix, strlen(suffix));
}

Error_t strdyn_append_fmt_(const ErrorInfo_t ei, strdyn_t *out, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    va_list args_copy;
    va_copy(args_copy, args);
    ssize_t len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    Error_t error = NO_ERRORS;
    do {
        if (len < 0) {
            error = error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
            break;
        }
        else if (len == 0) {
            break;
        }

        strdyn_impl_t *c = container_of_strdyn(*out);
        if (c->length > (SIZE_MAX - (size_t)len) - 1) {
            error = error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "sum of sizes overflows"});
            break;
        }
        else if ((error = strdyn_update_capacity(ei, out, c->length + (size_t)len + 1)).tag != ERROR_NONE) {
            break;
        }
        c = container_of_strdyn(*out);

        ssize_t written = 0;
        if ((written = vsnprintf(c->buf + c->length, (size_t)len + 1, format, args)) < 0) {
            error = error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
            break;
        }
        else if (written != len) {
            error = error_format_location(
                ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "inconsistent lengths observed!"});
            break;
        }
        c->length += (size_t)written;
    } while (false);

    va_end(args);
    return error;
}

Error_t strdyn_append_fmt_len_(const ErrorInfo_t ei, strdyn_t *out, const size_t max_len, const char *format, ...)
{
    if (max_len == 0) {
        return NO_ERRORS;
    }

    strdyn_impl_t *c = container_of_strdyn(*out);
    if (c->length > (SIZE_MAX - max_len) - 1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "sum of sizes overflows"});
    }

    const Error_t error = strdyn_update_capacity(ei, out, c->length + max_len);
    if (error.tag != ERROR_NONE) {
        return error;
    }
    c = container_of_strdyn(*out);

    va_list args;
    va_start(args, format);

    ssize_t written = 0;
    if ((written = vsnprintf(c->buf + c->length, max_len + 1, format, args)) < 0) {
        va_end(args);
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    const size_t appended = (written >= (ssize_t)max_len) ? max_len : (size_t)written;
    c->length += appended;

    va_end(args);
    return NO_ERRORS;
}
