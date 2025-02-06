
#include <assert.h>
#include <string.h>

#include "connection_tcp.h"
#include "error.h"
#include "http_server.h"

static Error_t advance_count(const ErrorInfo_t ei, const size_t max_buf_len, size_t *out_count, const size_t offset)
{
    *out_count += offset;
    if (*out_count > max_buf_len) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "parsing error: max_buf_len exceeded"});
    }
    return NO_ERRORS;
}

static Error_t skip_space(const ErrorInfo_t ei, const size_t max_buf_len, const char *buf, size_t *out_count)
{
    if (!strncmp(&buf[*out_count], " ", sizeof(" "))) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "parsing error: missing expected space"});
    }
    return advance_count(ei, max_buf_len, out_count, sizeof(" ") - 1);
}

static Error_t tokenize_protocol(
    const ErrorInfo_t ei,
    const size_t max_buf_len,
    const char *buf,
    size_t *out_count,
    struct tokenized_request *out_req)
{
    size_t start_count = *out_count;
    Error_t e = NO_ERRORS;

    if (!strncmp(&buf[*out_count], "HTTP", sizeof("HTTP"))) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "parsing error: message does not begin with 'HTTP'"});
    }
    e = advance_count(ei, max_buf_len, out_count, sizeof("HTTP") - 1);
    if (e.tag != ERROR_NONE) return e;

    out_req->protocol_name = csview_with_n(&buf[start_count], (long)(*out_count - start_count));

    if (!strncmp(&buf[*out_count], "/", sizeof("/"))) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "parsing error: missing expected / after HTTP"});
    }
    e = advance_count(ei, max_buf_len, out_count, sizeof("/") - 1);
    if (e.tag != ERROR_NONE) return e;

    start_count = *out_count;

    if (!strncmp(&buf[*out_count], "1.1", sizeof("1.1"))) { // TODO: support other versions / return error appropiately
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "parsing error: message does not begin with 'HTTP/1.1'"});
    }
    e = advance_count(ei, max_buf_len, out_count, sizeof("1.1") - 1);
    if (e.tag != ERROR_NONE) return e;

    out_req->protocol_ver = csview_with_n(&buf[start_count], (long)(*out_count - start_count));

    return NO_ERRORS;
}

static Error_t tokenize_status_code(
    const ErrorInfo_t ei,
    const size_t max_buf_len,
    const char *buf,
    size_t *out_count,
    struct tokenized_request *out_req)
{
}

Error_t tokenize_request_(
    const ErrorInfo_t ei,
    const size_t max_buf_len,
    const char *buf,
    struct tokenized_request *out_req,
    struct headers_htable *out_headers)
{
    size_t count = 0;
    Error_t e = NO_ERRORS;

    e = tokenize_protocol(ei, max_buf_len, buf, &count, out_req);
    if (e.tag != ERROR_NONE) return e;

    e = skip_space(ei, max_buf_len, buf, &count);
    if (e.tag != ERROR_NONE) return e;
}

/*

Error_t send_service_unavailable_(const ErrorInfo_t ei, const int conn_fd)
{
    assert(conn_fd >= 0);
    static const char msg[] = "HTTP/1.1 403 Forbidden\r\n"
                              "Content-Type: text/html\r\n"
                              "Content-Length: 0\r\n"
                              "\r\n";
    return bytes_sendall_(ei, 0, conn_fd, sizeof(msg), msg);
}

Error_t send_forbidden_packet_(const ErrorInfo_t ei, const int conn_fd)
{
    assert(conn_fd >= 0);
    static const char msg[] = "HTTP/1.1 403 Forbidden\r\n"
                              "Content-Type: text/html\r\n"
                              "Content-Length: 0\r\n"
                              "\r\n";
    return bytes_sendall_(ei, 0, conn_fd, sizeof(msg), msg);
}

Error_t send_bad_request_(const ErrorInfo_t ei, const int conn_fd)
{
    assert(conn_fd >= 0);
    static const char msg[] = "HTTP/1.1 400 Bad Request\r\n"
                              "Content-Type: text/html\r\n"
                              "Content-Length: 0\r\n"
                              "\r\n";
    return bytes_sendall_(ei, 0, conn_fd, sizeof(msg), msg);
}

*/
