#pragma once

#include "error.h"

#include "sds.h"
#include "types/strtable.h"
#include "types/strview.h"

#include <stdint.h>

struct RequestLine {
    strview method;
    strview url;
    strview protocol_name;
    strview protocol_version;
};

struct HTTPHeader {
    strview field_name;
    strview field_content;
};

struct StatusLine {
    strview http_version;
    strview status_code;
    strview status_desc;
};

/**
 * Tokenize request line
 */
Error_t tokenize_request_line_(const ErrorInfo_t ei, const strview line, struct RequestLine *out);

/**
 * Tokenize header
 */
Error_t tokenize_header_(const ErrorInfo_t ei, const strview line, struct HTTPHeader *out);

/**
 * Assemble response header with 'CLRS' as ending bytes
 */
Error_t assemble_header_(const ErrorInfo_t ei, struct StatusLine status, const strtable *headers, sds *out_buf);

#define tokenize_request_line(...) tokenize_request_line_(ERROR_INFO("tokenize_request_line"), __VA_ARGS__)
#define tokenize_header(...)       tokenize_header_(ERROR_INFO("tokenize_header"), __VA_ARGS__)
#define assemble_header(...)       assemble_header_(ERROR_INFO("assemble_header"), __VA_ARGS__)
