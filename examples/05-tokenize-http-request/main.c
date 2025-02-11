
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connection_tcp.h"
#include "http_server.h"

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

    e = open_tcp_server(port, &server_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    e = open_tcp_client_connection(server_fd, &conn_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    char msgbuf[4096] = {0};
    struct BufferedReader reader = {0};
    buffered_reader_init(&reader, conn_fd, sizeof(msgbuf), msgbuf);

    char linebuf[1024] = {0};
    size_t line_len = 0;
    printf("Recieving request:\n");

    printf(" - request line:\n");
    e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
    if (e.tag != ERROR_NONE) goto on_error;

    struct request_line request_line = {0};
    e = tokenize_request_line(line_len, linebuf, &request_line);
    printf("  - method: %.*s\n", (int)request_line.method.size, request_line.method.buf);
    printf("  - url: %.*s\n", (int)request_line.url.size, request_line.url.buf);
    printf("  - protocol name: %.*s\n", (int)request_line.protocol_name.size, request_line.protocol_name.buf);
    printf("  - protocol version: %.*s\n", (int)request_line.protocol_version.size, request_line.protocol_version.buf);

    do {
        e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
        if (e.tag != ERROR_NONE) goto on_error;

        struct header header = {0};
        e = tokenize_header(line_len, linebuf, &header);
        if (line_len <= 2) {
            break;
        }
        printf(" - header field(%.*s)\n", (int)header.field_name.size, header.field_name.buf);
        printf("  - value: %.*s\n", (int)header.field_value.size, header.field_value.buf);
    } while (true); // expecting no payload and no errors
    printf("\n");

    printf("Sending response...\n");
    e = bytes_sendall(conn_fd, strlen(RESPONSE_200_OK), RESPONSE_200_OK);
    if (e.tag != ERROR_NONE) goto on_error;

on_error:
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
