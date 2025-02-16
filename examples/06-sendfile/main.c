#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connection_tcp.h"
#include "message.h"

#define i_static
#define STC_CSTR_IO
#include <STC/include/stc/cstr.h>

#include "types/csview.h"

Error_t send_file_entity(const int conn_fd, const char *content_type, const char *content_length, const char *filepath)
{
    int file_handle = open(filepath, O_RDONLY);
    if (file_handle < 0) {
        return error_format_location(
            ERROR_INFO("open_file_and_get_file_size"), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }

    struct csview_htable *headers = csview_htable_create(2);
    csview_htable_update(headers, c_sv("Content-Type"), csview_from(content_type));
    csview_htable_update(headers, c_sv("Content-Length"), csview_from(content_length));

    const struct StatusLine status = {
        .http_version = csview_from("1.0"), .status_code = csview_from("200"), .status_desc = csview_from("OK")};

    size_t out_buf_len = 0;
    char *out_buf = NULL;
    Error_t e = NO_ERRORS;

    e = assemble_response_header(status, headers, &out_buf_len, &out_buf);
    if (e.tag != ERROR_NONE) goto cleanup1;

    e = bytes_sendall(conn_fd, out_buf_len, out_buf);
    if (e.tag != ERROR_NONE) goto cleanup1;

    e = bytes_sendfile(conn_fd, file_handle, 100 * (int)1e+6);
    if (e.tag != ERROR_NONE) goto cleanup1;

cleanup1:
    if (file_handle >= 0) {
        close(file_handle);
    }
    if (out_buf != NULL) {
        free(out_buf);
    }
    if (headers != NULL) {
        csview_htable_destroy(headers);
    }
    return e;
}

struct Route {
    cstr filepath;
    const char *content_type;
    cstr content_length;
};

#define NAME               routes_htable
#define KEY_TYPE           csview
#define VALUE_TYPE         struct Route
#define KEY_IS_EQUAL(a, b) (csview_equals_sv((a), (b)))
#define HASH_FUNCTION(key) (fnvhash_32((uint8_t *)(key).buf, (size_t)(key).size))
#define TYPE_DEFINITIONS
#define FUNCTION_DEFINITIONS
#define FUNCTION_LINKAGE static inline
#include <dsa-c/fhashtable/fhashtable_template.h>

struct ClientHandler {
    struct routes_htable *routes;
};

static const char RESPONSE_403_BAD_REQUEST[] = "HTTP/1.0 403 Bad Request\r\n"
                                               "Content-Type: text/plain\r\n"
                                               "Content-Length: 15\r\n"
                                               "\r\n"
                                               "403 Bad Request";

Error_t open_file_and_get_file_size(const char *filepath, cstr *out_file_size_str)
{
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return error_format_location(
            ERROR_INFO("open_file_and_get_file_size"), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        return error_format_location(
            ERROR_INFO("open_file_and_get_file_size"), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    const ssize_t size = ftell(fp);
    if (size == -1) {
        return error_format_location(
            ERROR_INFO("open_file_and_get_file_size"), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    *out_file_size_str = cstr_from_fmt("%u", size);
    return NO_ERRORS;
}

Error_t init_client_handler(struct ClientHandler *handler, const char *rootpath)
{
    handler->routes = routes_htable_create(64);
    if (!handler->routes) {
        return error_format_location(
            ERROR_INFO("init_response_handler"), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    struct Route r = {0};
    Error_t e = NO_ERRORS;

    r = (struct Route){0};
    r.filepath = cstr_from(rootpath);
    cstr_append_fmt(&r.filepath, "/index.html");
    r.content_type = "text/html";
    e = open_file_and_get_file_size(cstr_str(&r.filepath), &r.content_length);
    if (e.tag != ERROR_NONE) return e;
    routes_htable_update(handler->routes, c_sv("/"), r);

    r.filepath = cstr_clone(r.filepath);
    r.content_length = cstr_clone(r.content_length);
    routes_htable_update(handler->routes, c_sv("/index.html"), r);

    r = (struct Route){0};
    r.filepath = cstr_from(rootpath);
    cstr_append_fmt(&r.filepath, "/images/cat.jpg");
    r.content_type = "image/jpeg";
    e = open_file_and_get_file_size(cstr_str(&r.filepath), &r.content_length);
    if (e.tag != ERROR_NONE) return e;
    routes_htable_update(handler->routes, c_sv("/images/cat.jpg"), r);

    r.filepath = cstr_clone(r.filepath);
    r.content_length = cstr_clone(r.content_length);
    routes_htable_update(handler->routes, c_sv("/epic-cat"), r);

    return e;
}

void destroy_client_handler(struct ClientHandler *handler)
{
    {
        csview key;
        struct Route value;
        size_t idx;
        (void)(key);
        FHASHTABLE_FOR_EACH(handler->routes, idx, key, value)
        {
            cstr_drop(&value.content_length);
            cstr_drop(&value.filepath);
        }
    }
    routes_htable_destroy(handler->routes);
}

Error_t handle_client(const int conn_fd, struct ClientHandler *handler)
{
    char request_buf[4096] = {0};
    struct BufferedReader reader;
    buffered_reader_init(&reader, conn_fd, sizeof(request_buf), request_buf);

    Error_t e = NO_ERRORS;
    char request_line_buf[1024] = {0};
    size_t request_line_len = 0;

    e = bytes_recvline(&reader, sizeof(request_line_buf), request_line_buf, &request_line_len);
    if (e.tag != ERROR_NONE) goto on_error;

    struct RequestLine request_line = {0};
    e = tokenize_request_line(request_line_len, request_line_buf, &request_line);
    if (e.tag != ERROR_NONE) goto on_error;

    /*
    printf("request line:\n");
    printf(" method: %.*s\n", (int)request_line.method.size, request_line.method.buf);
    printf(" url: %.*s\n", (int)request_line.url.size, request_line.url.buf);
    printf(" protocol name: %.*s\n", (int)request_line.protocol_name.size, request_line.protocol_name.buf);
    printf(" protocol version: %.*s\n", (int)request_line.protocol_version.size, request_line.protocol_version.buf);

    {
        csview key;
        struct Route value;
        size_t idx;
        (void)(key);
        FHASHTABLE_FOR_EACH(handler->routes, idx, key, value)
        {
            printf("route :\n");
            printf(" - %.*s", (int)key.size, key.buf);
            printf(
                " -> %s (%s bytes) of type %s\n",
                cstr_str(&value.filepath),
                cstr_str(&value.content_length),
                value.content_type);
        }
    }
    */

    struct HTTPHeader header = {0}; // do no validation / processing of the headers.
    do {
        char linebuf[1024] = {0};
        size_t line_len = 0;
        e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
        if (e.tag != ERROR_NONE) goto on_error;

        if (line_len <= 2) {
            break;
        }

        e = tokenize_header(line_len, linebuf, &header);
        if (e.tag != ERROR_NONE) goto on_error;
    } while (true);

    if (routes_htable_contains_key(handler->routes, request_line.url)) // no special processing
    {
        const struct Route route = *routes_htable_get_value_mut(handler->routes, request_line.url);
        return send_file_entity(
            conn_fd, route.content_type, cstr_str(&route.content_length), cstr_str(&route.filepath));
    }
    else {
        e.tag = ERROR_CUSTOM;
        e.custom_msg = "no matching routes! sending 403";
        e = error_format_location(ERROR_INFO("handle_response"), e);
    }

on_error:
    bytes_sendall(conn_fd, sizeof(RESPONSE_403_BAD_REQUEST) - 1, RESPONSE_403_BAD_REQUEST); // ignore any errors
    return e;
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        const char *program_name = (argc == 1) ? argv[0] : "<program>";
        fprintf(stderr, "usage: %s <port> <root-path>\n", program_name);
        return EXIT_FAILURE;
    }
    const char *port = (argc > 1) ? argv[1] : 0;
    const char *rootpath = (argc > 2) ? argv[2] : 0;

    char error_strbuf[512] = {0};

    int server_fd = -1;
    int conn_fd = -1;

    struct ClientHandler client_handler = {0};
    const Error_t client_handler_error = init_client_handler(&client_handler, rootpath);
    if (client_handler_error.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(client_handler_error, sizeof(error_strbuf), error_strbuf));
        return EXIT_FAILURE;
    }

    const Error_t server_open_error = open_tcp_server(port, &server_fd);
    if (server_open_error.tag != ERROR_NONE) {
        destroy_client_handler(&client_handler);
        printf("%s\n", error_stringify(server_open_error, sizeof(error_strbuf), error_strbuf));
        return EXIT_FAILURE;
    }

    while (true) {
        const Error_t client_open_error = open_tcp_client_connection(server_fd, &conn_fd);
        if (client_open_error.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(client_open_error, sizeof(error_strbuf), error_strbuf));
            continue;
        }

        const Error_t handle_response_error = handle_client(conn_fd, &client_handler);
        if (handle_response_error.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(handle_response_error, sizeof(error_strbuf), error_strbuf));
            close_socket(conn_fd);
            continue;
        }

        const Error_t client_close_error = close_socket(conn_fd);
        if (handle_response_error.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(client_close_error, sizeof(error_strbuf), error_strbuf));
        }
    }
    destroy_client_handler(&client_handler);
    return EXIT_SUCCESS;
}
