#pragma once

#include "error.h"

#include "types/strdyn.h"
#include "types/strtable.h"
#include "types/strview.h"

#include <stdint.h>

struct RequestLine {
    strview_t method;
    strview_t url;
    strview_t protocol_name;
    strview_t protocol_version;
};

struct HTTPHeader {
    strview_t field_name;
    strview_t field_content;
};

struct StatusLine {
    strview_t http_version;
    strview_t status_code;
    strview_t status_desc;
};

/**
 * Tokenize request line
 */
Error_t tokenize_request_line_(const ErrorInfo_t ei, const strview_t line, struct RequestLine *out);

/**
 * Tokenize header
 */
Error_t tokenize_header_(const ErrorInfo_t ei, const strview_t line, struct HTTPHeader *out);

/**
 * Assemble response header with 'CLRS' as ending bytes
 */
Error_t assemble_header_(const ErrorInfo_t ei, struct StatusLine status, const strtable_t *headers, strdyn_t *out_buf);

#define tokenize_request_line(...) tokenize_request_line_(ERROR_INFO("tokenize_request_line"), __VA_ARGS__)
#define tokenize_header(...)       tokenize_header_(ERROR_INFO("tokenize_header"), __VA_ARGS__)
#define assemble_header(...)       assemble_header_(ERROR_INFO("assemble_header"), __VA_ARGS__)
