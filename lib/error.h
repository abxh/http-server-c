#pragma once

#include <stddef.h>
#include <stdint.h>

enum ErrorTag {
    ERROR_NONE = 0,
    ERROR_UNHANDLED,
    ERROR_NULL_PARAM,
    ERROR_ERRNO,
    ERROR_GAI,
};

typedef struct Error {
    enum ErrorTag tag;
    char location[256];
    union {
        int errno_num;
        int gai_errcode;
        char null_param_name[32];
    };
} Error_t;

static const Error_t NO_ERRORS = {0};

char *error_stringify(Error_t error, size_t buf_size, char *out_buf);

void error_format_null_param_name(Error_t *error_ptr, const char *param_name);

void error_format_location(
    Error_t *error_ptr,
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename);

#define RETURN_IF_NULL(param, ...)                         \
    do {                                                   \
        if (param == NULL) {                               \
            Error_t error_;                                \
            error_.tag = ERROR_NULL_PARAM;                 \
            error_format_null_param_name(&error_, #param); \
            error_format_location(&error_, __VA_ARGS__);   \
            return error_;                                 \
        }                                                  \
    } while (0)
