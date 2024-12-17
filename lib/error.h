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
    char location[256];
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
