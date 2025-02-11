#pragma once

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum ErrorTag {
    ERROR_NONE = 0,
    ERROR_UNHANDLED,
    ERROR_NULL_PARAM,
    ERROR_CUSTOM,
    ERROR_ERRNO,
    ERROR_GAI,
};

typedef struct Error {
    enum ErrorTag tag;
    char location[128];
    union {
        int errno_num;
        int gai_errcode;
        char null_param_name[32];
        char *custom_msg;
    };
} Error_t;

typedef struct ErrorInfo {
    const char *funcname;
    const char *calleename;
    const uint64_t linenr;
    const char *filename;
} ErrorInfo_t;

static const Error_t NO_ERRORS = {0};

static const ErrorInfo_t DEFAULT_ERROR_INFO = {
    .funcname = "<funcname>", .calleename = "<calleename>", .linenr = 0, .filename = "<filename>"};

char *error_stringify(Error_t error, size_t buf_size, char *out_buf);

static inline Error_t error_format_null_param_name(Error_t error, const char *param_name)
{
    snprintf(error.null_param_name, sizeof error.null_param_name, "%s", param_name);
    return error;
}

static inline Error_t error_format_location(const ErrorInfo_t ei, Error_t error)
{
    snprintf(
        error.location,
        sizeof error.location,
        "%s() in %s() at line %" PRId64 " in file %s",
        ei.funcname,
        ei.calleename,
        ei.linenr,
        ei.filename);
    return error;
}

#define ERROR_INFO(funcname_)                                                                     \
    (struct ErrorInfo)                                                                            \
    {                                                                                             \
        .funcname = (funcname_), .calleename = __func__, .linenr = __LINE__, .filename = __FILE__ \
    }

#define RETURN_IF_NULL(ei, param)                                                                \
    do {                                                                                         \
        if ((param) == NULL) {                                                                   \
            return error_format_location(                                                        \
                (ei), error_format_null_param_name((Error_t){.tag = ERROR_NULL_PARAM}, #param)); \
        }                                                                                        \
    } while (0)
