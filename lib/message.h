#pragma once

#include "error.h"

#include "types/strtable.h"
#include "types/strview.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
 * Default dummy malloc function
 */
static inline void *malloc_default(void *context, size_t alignment, size_t size)
{
    (void)(context);
    (void)(alignment);
    return malloc(size);
}

/**
 * Assemble response header with 'CLRS' as ending bytes
 */
Error_t assemble_response_header_(
    const ErrorInfo_t ei,
    void *allocator_context,
    void *(*allocate)(void *context, size_t alignment, size_t size),
    struct StatusLine status,
    const strtable *headers,
    size_t *out_buf_len,
    char **out_buf);

#define tokenize_request_line(...) tokenize_request_line_(ERROR_INFO("tokenize_request_line"), __VA_ARGS__)
#define tokenize_header(...)       tokenize_header_(ERROR_INFO("tokenize_header"), __VA_ARGS__)
#define assemble_response_header(...) \
    assemble_response_header_(ERROR_INFO("assemble_response_header"), NULL, malloc_default, __VA_ARGS__)
#define assemble_response_header_with_allocator(...) \
    assemble_response_header_with_allocator_(ERROR_INFO("assemble_response_header_with_allocator"), __VA_ARGS__)
