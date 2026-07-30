
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

    strtable *headers = strtable_create(2);

    sds response_header = NULL;

    char msgbuf[4096] = {0};
    char linebuf[1024] = {0};
    size_t line_len = 0;
    struct BufferedReader reader = {0};

    struct RequestLine request_line = {0};
    sds rbody = sdsempty();

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

        rbody = sdscat(rbody, "request structure:\n");
        rbody = sdscat(rbody, " - request line:\n");
        rbody = sdscatprintf(rbody, "  - method: %.*s\n", (int)request_line.method.size, request_line.method.buf);
        rbody = sdscatprintf(rbody, "  - url: %.*s\n", (int)request_line.url.size, request_line.url.buf);
        // clang-format off
        rbody =
            sdscatprintf(rbody, "  - protocol name: %.*s\n",
            (int)request_line.protocol_name.size,
            request_line.protocol_name.buf);
        rbody =
            sdscatprintf(rbody, "  - protocol version: %.*s\n",
            (int)request_line.protocol_version.size,
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

            rbody = sdscatprintf(rbody, " - header field(%.*s)\n", (int)header.field_name.size, header.field_name.buf);
            rbody = sdscatprintf(rbody, "  - value: %.*s\n", (int)header.field_content.size, header.field_content.buf);
        } while (true); // expecting no payload and no errors

        rbody = sdscat(rbody, "\n");
        sds content_length_str = sdsfromlonglong((long long)sdslen(rbody));
        strtable_update(headers, strview_from_cstr("Content-Type"), strview_from_cstr("text/plain"));
        strtable_update(headers, strview_from_cstr("Content-Length"), strview_from_cstr(content_length_str));

        struct StatusLine status = {
            .http_version = strview_from_cstr("1.0"),
            .status_code = strview_from_cstr("200"),
            .status_desc = strview_from_cstr("OK"),
        };
        e = assemble_response_header(status, headers, &response_header);
        if (e.tag != ERROR_NONE) goto on_error;

        e = bytes_sendall(conn_fd, sdslen(response_header), response_header);
        if (e.tag != ERROR_NONE) goto on_error;

        e = bytes_sendall(conn_fd, sdslen(rbody), rbody);
        if (e.tag != ERROR_NONE) goto on_error;

        sdsfree(content_length_str);

        sdsclear(rbody);
        strtable_clear(headers);
        buffered_reader_flush(&reader);
        sdsclear(response_header);

        e = close_socket(conn_fd);
        if (e.tag != ERROR_NONE) goto on_error;
        conn_fd = -1;
    };

on_error:
    sdsfree(rbody);
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
