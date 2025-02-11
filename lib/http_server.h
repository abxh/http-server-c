#pragma once

#include "error.h"

#include "stc/csview.h"

struct request_line {
    csview method;
    csview url;
    csview protocol_name;
    csview protocol_version;
};

struct header {
    csview field_name;
    csview field_value;
};

extern const char RESPONSE_200_OK[];
extern const char RESPONSE_400_BAD_REQUEST[];
extern const char RESPONSE_403_FORBIDDEN[];
extern const char RESPONSE_404_NOT_FOUND[];
extern const char RESPONSE_500_INTERNAL_SERVER_ERROR[];
extern const char RESPONSE_501_NOT_IMPLEMENTED[];

/**
 * Tokenize request line
 */
Error_t tokenize_request_line_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct request_line *out);

/**
 * Tokenize request header
 */
Error_t tokenize_header_(const ErrorInfo_t ei, const size_t line_len, const char *line, struct header *out);

#define tokenize_request_line(...) tokenize_request_line_(ERROR_INFO("tokenize_request_line"), __VA_ARGS__)
#define tokenize_header(...) tokenize_header_(ERROR_INFO("tokenize_header"), __VA_ARGS__)
