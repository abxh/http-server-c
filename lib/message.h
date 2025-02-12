#pragma once

#include "error.h"

#include "types/csview.h"
#include "types/csview_htable.h"

#include <stdlib.h>
#include <string.h>

struct RequestLine {
    csview method;
    csview url;
    csview protocol_name;
    csview protocol_version;
};

struct HTTPHeader {
    csview field_name;
    csview field_value;
};

struct StatusLine {
    csview http_version;
    csview status_code;
    csview status_desc;
};

struct Response {
    struct StatusLine status;
    struct csview_htable *headers;
    csview body;
};

/**
 * Tokenize request line
 */
Error_t tokenize_request_line_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct RequestLine *out);

/**
 * Tokenize request header
 */
Error_t tokenize_header_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct HTTPHeader *out);

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
 * Assemble response
 */
Error_t assemble_response_(
    const ErrorInfo_t ei,
    void *allocator_context,
    void *(*allocate)(void *context, size_t alignment, size_t size),
    const struct Response *in,
    size_t *out_buf_len,
    char **out_buf);

#define tokenize_request_line(...) tokenize_request_line_(ERROR_INFO("tokenize_request_line"), __VA_ARGS__)
#define tokenize_header(...)       tokenize_header_(ERROR_INFO("tokenize_header"), __VA_ARGS__)
#define assemble_response(...)     assemble_response_(ERROR_INFO("assemble_response"), NULL, malloc_default, __VA_ARGS__)
#define assemble_response_with_allocator(...) \
    assemble_response_(ERROR_INFO("assemble_response_with_allocator"), __VA_ARGS__)
