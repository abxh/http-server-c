#pragma once

#include "error.h"
#include "error_utils.h"

#include "stc/csview.h"

#include "fnvhash.h"
#define NAME               headers_htable
#define KEY_TYPE           const char *
#define VALUE_TYPE         csview
#define KEY_IS_EQUAL(a, b) (strcmp((a), (b)->buf) == 0)
#define HASH_FUNCTION(key) (fnvhash_32_str(key))
#define TYPE_DEFINITIONS
#define FUNCTION_DEFINITIONS
#define FUNCTION_LINKAGE static inline
#include "fhashtable_template.h"

struct tokenized_request {
    csview protocol_name;
    csview protocol_ver;
    csview status_code;
    csview status_text;
    csview headers;
    csview body;
};

Error_t tokenize_request_(
    const ErrorInfo_t ei,
    const size_t max_buf_len,
    const char *buf,
    struct tokenized_request *out_req,
    struct headers_htable *out_headers);

Error_t handle_request_(const ErrorInfo_t ei, const int conn_fd, struct tokenized_request *req, struct headers_htable *headers);

#define parse_request(...)  parse_request_(ERROR_INFO("parse_request"), __VA_ARGS__)
#define handle_request(...) handle_request_(ERROR_INFO("handle_request"), __VA_ARGS__)
