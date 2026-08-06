#pragma once

#include "error.h"

typedef char *strdyn_t;

void strdyn_free(strdyn_t s);

size_t strdyn_length(const strdyn_t s);
void strdyn_clear(strdyn_t s);

Error_t strdyn_reserve_(const ErrorInfo_t ei, strdyn_t *out, const size_t len);
Error_t strdyn_empty_(const ErrorInfo_t ei, strdyn_t *out);

Error_t strdyn_append_len_(const ErrorInfo_t ei, strdyn_t *out, const char *suffix, const size_t suffix_len);
Error_t strdyn_append_(const ErrorInfo_t ei, strdyn_t *out, const char *suffix);

Error_t strdyn_append_fmt_(const ErrorInfo_t ei, strdyn_t *out, const char *format, ...);
Error_t strdyn_append_fmt_len_(const ErrorInfo_t ei, strdyn_t *out, const size_t max_len, const char *format, ...);

#define strdyn_reserve(...)        strdyn_reserve_(ERROR_INFO("strdyn_reserve"), __VA_ARGS__)
#define strdyn_empty(...)          strdyn_empty_(ERROR_INFO("strdyn_empty"), __VA_ARGS__)
#define strdyn_append_len(...)     strdyn_append_len_(ERROR_INFO("strdyn_append_len"), __VA_ARGS__)
#define strdyn_append(...)         strdyn_append_(ERROR_INFO("strdyn_append"), __VA_ARGS__)
#define strdyn_append_fmt(...)     strdyn_append_fmt_(ERROR_INFO("strdyn_append_fmt"), __VA_ARGS__)
#define strdyn_append_fmt_len(...) strdyn_append_fmt_len_(ERROR_INFO("strdyn_append_fmt"), __VA_ARGS__)
