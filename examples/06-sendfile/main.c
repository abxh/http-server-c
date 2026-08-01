#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "connection.h"
#include "connection_tcp.h"
#include "message.h"

#include "sds/sds.h"

struct Route {
    strview url_path;
    strview local_path;
};

struct RouteWithMetadata {
    struct Route route;
    sds abs_path;
    strview content_type;
    sds content_length_str;
};

static const struct Route routes[] = {
    {
        .url_path = STRVIEW("/"),
        .local_path = STRVIEW("/index.html"),
    },
    {
        .url_path = STRVIEW("/index.html"),
        .local_path = STRVIEW("/index.html"),
    },
    {
        .url_path = STRVIEW("/images/cat.jpg"),
        .local_path = STRVIEW("/images/cat.jpg"),
    },
    {
        .url_path = STRVIEW("/cat"),
        .local_path = STRVIEW("/images/cat.jpg"),
    },
};

static const char RESPONSE_403_BAD_REQUEST[] = "HTTP/1.0 403 Bad Request\r\n"
                                               "Content-Type: text/plain\r\n"
                                               "Content-Length: 15\r\n"
                                               "\r\n"
                                               "403 Bad Request";

Error_t send_file_entity(const int conn_fd, const char *content_type, const char *content_length, const char *filepath)
{
    int file_handle = open(filepath, O_RDONLY);
    if (file_handle < 0) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }

    struct strtable *headers = strtable_create(2);
    strtable_update(headers, STRVIEW_FROM("Content-Type"), strview_from_cstr(content_type));
    strtable_update(headers, STRVIEW_FROM("Content-Length"), strview_from_cstr(content_length));

    const struct StatusLine status = {
        .http_version = STRVIEW("1.0"),
        .status_code = STRVIEW("200"),
        .status_desc = STRVIEW("OK"),
    };

    sds out_buf = NULL;
    Error_t e = NO_ERRORS;

    e = assemble_response_header(status, headers, &out_buf);
    if (e.tag != ERROR_NONE) goto cleanup1;

    e = bytes_sendall(conn_fd, sdslen(out_buf), out_buf);
    if (e.tag != ERROR_NONE) goto cleanup1;

    e = bytes_sendfile(conn_fd, file_handle, 100 * (int)1e+6); // last parameter: max file size
    if (e.tag != ERROR_NONE) goto cleanup1;

cleanup1:
    if (file_handle >= 0) {
        close(file_handle);
    }
    if (out_buf != NULL) {
        sdsfree(out_buf);
    }
    if (headers != NULL) {
        strtable_destroy(headers);
    }
    return e;
}

#include <data-structures-c/fhashtable/fnvhash.h>

#define NAME               routes_htable
#define KEY_TYPE           strview
#define VALUE_TYPE         struct RouteWithMetadata
#define KEY_IS_EQUAL(a, b) (strview_equals((a), (b)))
#define HASH_FUNCTION(key) (fnvhash_32((uint8_t *)(key).buf, (size_t)(key).length))
#define TYPE_DEFINITIONS
#define FUNCTION_DEFINITIONS
#define FUNCTION_LINKAGE static inline
#include <data-structures-c/fhashtable/fhashtable_template.h>

struct ClientHandler {
    struct routes_htable *routes;
};

strview deduce_content_type(const strview filepath)
{
    strview extension = STRVIEW_EMPTY;
    if (!strview_find_lastc(filepath, '.', &extension)) {
        return STRVIEW_FROM("application/octet-stream");
    }
    if (strview_equals(extension, STRVIEW_FROM(".html"))) {
        return STRVIEW_FROM("text/html");
    }
    if (strview_equals(extension, STRVIEW_FROM(".jpg")) || strview_equals(extension, STRVIEW_FROM(".jpeg"))) {
        return STRVIEW_FROM("image/jpeg");
    }
    return STRVIEW_FROM("application/octet-stream");
}

Error_t open_file_and_get_file_size(const char *filepath, sds *out_file_size_str)
{
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    const ssize_t size = ftell(fp);
    if (size == -1) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    *out_file_size_str = sdsfromlonglong(size);
    return NO_ERRORS;
}

Error_t init_client_handler(struct ClientHandler *handler, const char *rootpath)
{
    handler->routes = routes_htable_create(sizeof(routes) / sizeof(*routes));
    if (!handler->routes) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    Error_t e = NO_ERRORS;

    for (size_t i = 0; i < sizeof(routes) / sizeof(*routes); i++) {
        const struct Route r = routes[i];
        struct RouteWithMetadata rm;
        rm.route = r;

        rm.abs_path = sdsnew(rootpath);
        CHECK_IF_NULL(ERROR_INFO(__func__), e, rm.abs_path);
        if (e.tag != ERROR_NONE) break;

        rm.abs_path = sdscatfmt(rm.abs_path, sdsnewlen(rm.route.local_path.buf, rm.route.local_path.length));
        CHECK_IF_NULL(ERROR_INFO(__func__), e, rm.abs_path);
        if (e.tag != ERROR_NONE) break;

        rm.content_type = deduce_content_type(strview_from_sized((const uint8_t *)rm.abs_path, sdslen(rm.abs_path)));

        e = open_file_and_get_file_size(rm.abs_path, &rm.content_length_str);
        if (e.tag != ERROR_NONE) break;

        routes_htable_update(handler->routes, r.url_path, rm);
    }
    return e;
}

void destroy_client_handler(struct ClientHandler *handler)
{
    {
        strview key;
        struct RouteWithMetadata value;
        size_t idx;
        FHASHTABLE_FOR_EACH(handler->routes, idx, key, value)
        {
            (void)(key);
            sdsfree(value.content_length_str);
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
    e = tokenize_request_line(strview_from_sized((uint8_t *)request_line_buf, request_line_len), &request_line);
    if (e.tag != ERROR_NONE) goto on_error;

    // printf("request line:\n");
    // printf(" method: %.*s\n", (int)request_line.method.size, request_line.method.buf);
    // printf(" url: %.*s\n", (int)request_line.url.size, request_line.url.buf);
    // printf(" protocol name: %.*s\n", (int)request_line.protocol_name.size, request_line.protocol_name.buf);
    // printf(" protocol version: %.*s\n", (int)request_line.protocol_version.size, request_line.protocol_version.buf);
    //
    // {
    //     strview key;
    //     struct Route value;
    //     size_t idx;
    //     (void)(key);
    //     FHASHTABLE_FOR_EACH(handler->routes, idx, key, value)
    //     {
    //         printf("route :\n");
    //         printf(" - %.*s", (int)key.size, key.buf);
    //         printf(" -> %s (%s bytes) of type %s\n", value.filepath, value.content_length, value.content_type.buf);
    //     }
    // }

    // do no validation / processing of the headers:
    struct HTTPHeader header = {0};
    do {
        char linebuf[1024] = {0};
        size_t line_len = 0;
        e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
        if (e.tag != ERROR_NONE) goto on_error;

        if (line_len == 0 || strncmp(linebuf, "\r\n", line_len)) {
            break;
        }

        e = tokenize_header(strview_from_sized((uint8_t *)linebuf, line_len), &header);
        if (e.tag != ERROR_NONE) goto on_error;
    } while (true);

    // no special processing:
    struct RouteWithMetadata *route = routes_htable_get_value_mut(handler->routes, request_line.url);
    if (route) {
        return send_file_entity(
            conn_fd, (const char *)route->content_type.buf, route->content_length_str, route->abs_path);
    }
    else {
        e.tag = ERROR_CUSTOM;
        e.custom_msg = "no matching routes! sending 403";
        e = error_format_location(ERROR_INFO("handle_client"), e);
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

        const Error_t handle_client_error = handle_client(conn_fd, &client_handler);
        if (handle_client_error.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(handle_client_error, sizeof(error_strbuf), error_strbuf));
            close_socket(conn_fd);
            conn_fd = -1;
            continue;
        }

        const Error_t client_close_error = close_socket(conn_fd);
        if (client_close_error.tag != ERROR_NONE) {
            printf("%s\n", error_stringify(client_close_error, sizeof(error_strbuf), error_strbuf));
        }
    }
    destroy_client_handler(&client_handler);
    return EXIT_SUCCESS;
}
