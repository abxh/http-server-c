#pragma once

#include "error.h"

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
