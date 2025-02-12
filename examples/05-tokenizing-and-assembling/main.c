
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define i_static
#define STC_CSTR_IO
#include "stc/cstr.h"

#include "connection.h"
#include "connection_tcp.h"
#include "message.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        const char *program_name = (argc == 1) ? argv[0] : "<program>";
        fprintf(stderr, "usage: %s <port>\n", program_name);
        return EXIT_FAILURE;
    }
    const char *port = (argc > 1) ? argv[1] : 0;

    Error_t e = NO_ERRORS;
    int return_status = EXIT_SUCCESS;
    char strbuf[512] = {0};

    int server_fd = -1;
    int conn_fd = -1;

    struct csview_htable *headers = NULL;
    size_t response_str_len = 0;
    char *response_str = NULL;

    e = open_tcp_server(port, &server_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    e = open_tcp_client_connection(server_fd, &conn_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    char msgbuf[4096] = {0};
    struct BufferedReader reader = {0};
    buffered_reader_init(&reader, conn_fd, sizeof(msgbuf), msgbuf);

    char linebuf[1024] = {0};
    size_t line_len = 0;
    e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
    if (e.tag != ERROR_NONE) goto on_error;

    struct RequestLine request_line = {0};
    e = tokenize_request_line(line_len, linebuf, &request_line);

    // check e

    cstr response_body = cstr_init();
    cstr_append(&response_body, "request structure:\n");
    cstr_append(&response_body, " - request line:\n");
    cstr_append_fmt(&response_body, "  - method: %.*s\n", (int)request_line.method.size, request_line.method.buf);
    cstr_append_fmt(&response_body, "  - url: %.*s\n", (int)request_line.url.size, request_line.url.buf);
    cstr_append_fmt(
        &response_body,
        "  - protocol name: %.*s\n",
        (int)request_line.protocol_name.size,
        request_line.protocol_name.buf);
    cstr_append_fmt(
        &response_body,
        "  - protocol version: %.*s\n",
        (int)request_line.protocol_version.size,
        request_line.protocol_version.buf);

    do {
        e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
        if (e.tag != ERROR_NONE) goto on_error;

        struct HTTPHeader header = {0};
        e = tokenize_header(line_len, linebuf, &header);
        if (line_len <= 2) {
            break;
        }
        cstr_append_fmt(&response_body, " - header field(%.*s)\n", (int)header.field_name.size, header.field_name.buf);
        cstr_append_fmt(&response_body, "  - value: %.*s\n", (int)header.field_value.size, header.field_value.buf);
    } while (true); // expecting no payload and no errors
    cstr_append_fmt(&response_body, "\n");

    cstr content_length_str = cstr_from_fmt("%d", cstr_size(&response_body));
    headers = csview_htable_create(2);
    csview_htable_update(headers, c_sv("Content-Type"), c_sv("text/plain"));
    csview_htable_update(headers, c_sv("Content-Length"), csview_from(cstr_str(&content_length_str)));
    struct StatusLine status = {
        .http_version = csview_from("1.0"), .status_code = csview_from("200"), .status_desc = csview_from("OK")};
    struct Response response = {.status = status, .headers = headers, .body = csview_from(cstr_str(&response_body))};

    e = assemble_response(&response, &response_str_len, &response_str);
    if (e.tag != ERROR_NONE) goto on_error;

    e = bytes_sendall(conn_fd, response_str_len, response_str);
    if (e.tag != ERROR_NONE) goto on_error;

on_error:
    if (response_body.lon.data != NULL) {
        cstr_drop(&response_body);
    }
    if (content_length_str.lon.data != NULL) {
        cstr_drop(&content_length_str);
    }
    if (headers != NULL) {
        csview_htable_destroy(headers);
    }
    if (response_str != NULL) {
        free(response_str);
    }
    if (e.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(e, sizeof(strbuf), strbuf));
        return_status = EXIT_FAILURE;
    }
    if (conn_fd != -1) {
        e = close_socket(conn_fd);
        if (e.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(e, sizeof(strbuf), strbuf));
            return_status = EXIT_FAILURE;
        }
    }
    if (server_fd != -1) {
        e = close_socket(server_fd);
        if (e.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(e, sizeof(strbuf), strbuf));
            return_status = EXIT_FAILURE;
        }
    }
    return return_status;
}
