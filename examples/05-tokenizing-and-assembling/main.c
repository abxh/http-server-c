
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sds/sds.h"

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
    char error_strbuf[512] = {0};

    int server_fd = -1;
    int conn_fd = -1;

    sds response_header = NULL;

    char msgbuf[4096] = {0};
    char linebuf[1024] = {0};
    size_t line_len = 0;
    struct BufferedReader reader = {0};

    struct RequestLine request_line = {0};
    sds body = sdsempty();
    sds content_length_str = sdsempty();
    strtable *headers = strtable_create(2);

    e = open_tcp_server(port, &server_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    while (true) {
        e = open_tcp_client_connection(server_fd, &conn_fd);
        if (e.tag != ERROR_NONE) goto on_error;

        buffered_reader_init(&reader, conn_fd, sizeof(msgbuf), msgbuf);
        e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
        if (e.tag != ERROR_NONE) goto on_error;

        e = tokenize_request_line(strview_from_sized((uint8_t *)linebuf, line_len), &request_line);
        if (e.tag != ERROR_NONE) goto on_error; // should ideally send a error response to user here

        body = sdscat(body, "request structure:\n");
        body = sdscat(body, " - request line:\n");
        body = sdscatprintf(body, "  - method: %.*s\n", (int)request_line.method.length, request_line.method.buf);
        body = sdscatprintf(body, "  - url: %.*s\n", (int)request_line.url.length, request_line.url.buf);
        // clang-format off
        body =
            sdscatprintf(body, "  - protocol name: %.*s\n",
            (int)request_line.protocol_name.length,
            request_line.protocol_name.buf);
        body =
            sdscatprintf(body, "  - protocol version: %.*s\n",
            (int)request_line.protocol_version.length,
            request_line.protocol_version.buf);
        // clang-format on

        do {
            e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
            if (e.tag != ERROR_NONE) goto on_error;

            if (line_len <= 2) {
                break;
            }

            struct HTTPHeader header = {0};
            e = tokenize_header(strview_from_sized((uint8_t *)linebuf, line_len), &header);
            if (e.tag != ERROR_NONE) goto on_error;

            body = sdscatprintf(body, " - header field(%.*s)\n", (int)header.field_name.length, header.field_name.buf);
            body = sdscatprintf(body, "  - value: %.*s\n", (int)header.field_content.length, header.field_content.buf);
        } while (true); // expecting no payload and no errors

        body = sdscat(body, "\n");
        content_length_str = sdscatprintf(content_length_str, "%zu", sdslen(body));

        strtable_update(headers, STRVIEW_FROM("Content-Type"), STRVIEW_FROM("text/plain"));
        strtable_update(headers, STRVIEW_FROM("Content-Length"), strview_from_cstr(content_length_str));

        struct StatusLine status = {
            .http_version = STRVIEW("1.0"),
            .status_code = STRVIEW("200"),
            .status_desc = STRVIEW("OK"),
        };
        e = assemble_response_header(status, headers, &response_header);
        if (e.tag != ERROR_NONE) goto on_error;

        e = bytes_sendall(conn_fd, sdslen(response_header), response_header);
        if (e.tag != ERROR_NONE) goto on_error;

        e = bytes_sendall(conn_fd, sdslen(body), body);
        if (e.tag != ERROR_NONE) goto on_error;

        sdsclear(body);
        sdsclear(content_length_str);
        strtable_clear(headers);
        buffered_reader_flush(&reader);
        sdsclear(response_header);

        e = close_socket(conn_fd);
        if (e.tag != ERROR_NONE) goto on_error;
        conn_fd = -1;
    };

on_error:
    sdsfree(body);
    sdsfree(content_length_str);
    strtable_destroy(headers);
    sdsfree(response_header);

    if (e.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(e, sizeof(error_strbuf), error_strbuf));
        return_status = EXIT_FAILURE;
    }
    if (conn_fd != -1) {
        e = close_socket(conn_fd);
        if (e.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(e, sizeof(error_strbuf), error_strbuf));
            return_status = EXIT_FAILURE;
        }
    }
    if (server_fd != -1) {
        e = close_socket(server_fd);
        if (e.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(e, sizeof(error_strbuf), error_strbuf));
            return_status = EXIT_FAILURE;
        }
    }
    return return_status;
}
