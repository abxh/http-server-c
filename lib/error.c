#include "error.h"

#include <inttypes.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

char *error_stringify(Error_t error, size_t buf_size, char *out_buf)
{
    switch (error.tag) {
    case ERROR_NONE:
        snprintf(out_buf, buf_size, "No errors.");
        break;
    case ERROR_UNHANDLED:
        snprintf(out_buf, buf_size, "%s: unhandled case", error.location);
        break;
    case ERROR_NULL_PARAM:
        snprintf(out_buf, buf_size, "%s: parameter '%s' is null", error.location, error.null_param_name);
        break;
    case ERROR_ERRNO:
        snprintf(out_buf, buf_size, "%s: %s", error.location, strerror(error.errno_num));
        break;
    case ERROR_GAI:
        snprintf(out_buf, buf_size, "%s: %s", error.location, gai_strerror(error.gai_errcode));
        break;
    case ERROR_CUSTOM:
        snprintf(out_buf, buf_size, "%s: %s", error.location, error.custom_msg);
        break;
    }
    return out_buf;
}

Error_t error_format_null_param_name(Error_t error, const char *param_name)
{
    snprintf(error.null_param_name, sizeof error.null_param_name, "%s", param_name);
    return error;
}

Error_t error_format_location(Error_t error, ErrorInfo_t ei)
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
