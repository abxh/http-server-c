
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connection_tcp.h"

// some help:
// https://stackoverflow.com/questions/22077802/simple-c-example-of-doing-an-http-post-and-consuming-the-response

void chr_cstrlit(unsigned char u, char *buffer, size_t buflen)
{
    // source:
    // https://stackoverflow.com/questions/7369344/how-to-unescape-strings-in-c-c

    if (buflen < 2) {
        *buffer = '\0';
    }
    else if (isprint(u) && u != '\'' && u != '\"' && u != '\\' && u != '\?') {
        sprintf(buffer, "%c", u);
    }
    else if (buflen < 3) {
        *buffer = '\0';
    }
    else {
        switch (u) {
        case '\a':
            strcpy(buffer, "\\a");
            break;
        case '\b':
            strcpy(buffer, "\\b");
            break;
        case '\f':
            strcpy(buffer, "\\f");
            break;
        case '\n':
            strcpy(buffer, "\\n");
            break;
        case '\r':
            strcpy(buffer, "\\r");
            break;
        case '\t':
            strcpy(buffer, "\\t");
            break;
        case '\v':
            strcpy(buffer, "\\v");
            break;
        case '\\':
            strcpy(buffer, "\\\\");
            break;
        case '\'':
            strcpy(buffer, "\\'");
            break;
        case '\"':
            strcpy(buffer, "\\\"");
            break;
        case '\?':
            strcpy(buffer, "\\\?");
            break;
        default:
            if (buflen < 5) {
                *buffer = '\0';
            }
            else {
                sprintf(buffer, "\\%03o", u);
            }
            break;
        }
    }
}

void str_cstrlit(const char *str, char *buffer, size_t buflen)
{
    // source:
    // https://stackoverflow.com/questions/7369344/how-to-unescape-strings-in-c-c

    unsigned char u;
    size_t len;

    while ((u = (unsigned char)*str++) != '\0') {
        chr_cstrlit(u, buffer, buflen);
        if ((len = strlen(buffer)) == 0) return;
        buffer += len;
        buflen -= len;
    }
    *buffer = '\0';
}

const char dummy_msg[] = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: 17\r\n"
                         "\r\n"
                         "Hello Web from C!";

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
    e = buffered_reader_init(&reader, conn_fd, sizeof(msgbuf), msgbuf);
    if (e.tag != ERROR_NONE) goto on_error;

    char linebuf[1024] = {0};
    char linebuf_escaped[1024] = {0};
    size_t line_len = 0;
    printf("Got reponse:\n");
    do {
        e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
        if (e.tag != ERROR_NONE) goto on_error;
        str_cstrlit(linebuf, linebuf_escaped, sizeof(linebuf_escaped));
        printf("%4zu: %s\n", line_len, linebuf_escaped);
    } while (line_len > 0 && strcmp(linebuf, "\r\n") != 0);
    printf("\n");

    printf("Sending response:\n");
    char *dup = strdup(dummy_msg);
    char *ptr = dup, *end = dup;
    while (ptr != NULL) {
        strsep(&end, "\n");
        str_cstrlit(ptr, linebuf_escaped, sizeof(linebuf_escaped));
        printf("%4lu: %s%s\n", strlen(ptr) + (end != NULL ? 1 : 0), linebuf_escaped, end != NULL ? "\\n" : "");
        ptr = end;
    }
    free(dup);

    e = bytes_sendall(conn_fd, sizeof(dummy_msg), dummy_msg);
    if (e.tag != ERROR_NONE) goto on_error;

on_error:
    if (e.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(e, sizeof(strbuf), strbuf));
        return_status = EXIT_FAILURE;
    }
    if (conn_fd != -1) {
        const Error_t error_close_conn = close_socket(conn_fd);
        if (error_close_conn.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(error_close_conn, sizeof(strbuf), strbuf));
            return_status = EXIT_FAILURE;
        }
    }
    if (server_fd != -1) {
        const Error_t error_close_server = close_socket(server_fd);
        if (error_close_server.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(error_close_server, sizeof(strbuf), strbuf));
            return_status = EXIT_FAILURE;
        }
    }
    return return_status;
}
