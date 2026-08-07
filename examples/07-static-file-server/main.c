
#include "message.h"

#include <connection.h>
#include <connection_tcp.h>
#include <linux/limits.h>
#include <types/strdyn.h>
#include <types/strtable.h>
#include <types/strview.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

struct ClientHandler {
    char rootpath_[PATH_MAX];
    strview_t rootpath;

    strtable_t *mime_table;
    strview_t default_mime_type;
};

bool file_exists(const char *filename)
{
    // https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
    struct stat buffer;
    return (stat(filename, &buffer) == 0);
}

Error_t open_file_and_get_file_size(const char *filepath, strdyn_t *out_file_size_str)
{
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    const ssize_t size = ftell(fp);
    if (size == -1) {
        fclose(fp);
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    Error_t e;
    e = strdyn_empty(out_file_size_str);
    if (e.tag != ERROR_NONE) {
        fclose(fp);
        return e;
    }
    e = strdyn_append_fmt(out_file_size_str, "%zu", size);
    if (e.tag != ERROR_NONE) {
        fclose(fp);
        return e;
    }
    if (fclose(fp) == -1) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t
send_file_response(const int conn_fd, const char *content_type, const char *content_length, const char *filepath)
{
    int file_handle = open(filepath, O_RDONLY);
    if (file_handle < 0) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }

    strtable_t *headers = strtable_create(2);
    strtable_update(headers, STRVIEW_FROM("Content-Type"), strview_from_cstr(content_type));
    strtable_update(headers, STRVIEW_FROM("Content-Length"), strview_from_cstr(content_length));

    const struct StatusLine status = {
        .http_version = STRVIEW("1.0"),
        .status_code = STRVIEW("200"),
        .status_desc = STRVIEW("OK"),
    };

    strdyn_t out_buf = NULL;
    Error_t e = NO_ERRORS;

    e = assemble_header(status, headers, &out_buf);
    if (e.tag != ERROR_NONE) goto cleanup1;

    e = bytes_sendall(conn_fd, strdyn_length(out_buf), out_buf);
    if (e.tag != ERROR_NONE) goto cleanup1;

    e = bytes_sendfile(conn_fd, file_handle, 100 * (int)1e+6); // last parameter: max file size
    if (e.tag != ERROR_NONE) goto cleanup1;

cleanup1:
    if (file_handle >= 0) {
        close(file_handle);
    }
    if (out_buf != NULL) {
        strdyn_free(out_buf);
    }
    if (headers != NULL) {
        strtable_destroy(headers);
    }
    return e;
}

Error_t init_mime_table(struct ClientHandler *handler)
{
    handler->mime_table = strtable_create(16);
    if (!handler->mime_table) {
        return error_format_location(ERROR_INFO(__func__), (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    handler->default_mime_type = STRVIEW_FROM("application/octet-stream");
    strtable_update(handler->mime_table, STRVIEW_FROM(".html"), STRVIEW_FROM("text/html"));
    strtable_update(handler->mime_table, STRVIEW_FROM(".css"), STRVIEW_FROM("text/css"));
    strtable_update(handler->mime_table, STRVIEW_FROM(".js"), STRVIEW_FROM("text/javascript"));
    strtable_update(handler->mime_table, STRVIEW_FROM(".txt"), STRVIEW_FROM("text/plain"));
    strtable_update(handler->mime_table, STRVIEW_FROM(".jpg"), STRVIEW_FROM("image/jpeg"));
    strtable_update(handler->mime_table, STRVIEW_FROM(".jpeg"), STRVIEW_FROM("image/jpg"));
    strtable_update(handler->mime_table, STRVIEW_FROM(".ico"), STRVIEW_FROM("image/vnd.microsoft.icon"));
    return NO_ERRORS;
}

const char *get_mime_type(struct ClientHandler *handler, const char *filepath)
{
    strview_t extension = STRVIEW_EMPTY;
    if (!strview_find_lastc(strview_from_cstr(filepath), '.', &extension)) {
        return (const char *)handler->default_mime_type.buf;
    }
    return (const char *)strtable_get_value(handler->mime_table, extension, handler->default_mime_type).buf;
}

Error_t init_client_handler(struct ClientHandler *handler, const char *rootpath)
{
    handler->rootpath = strview_from_cstr(realpath(rootpath, handler->rootpath_));
    return init_mime_table(handler);
}

void destroy_client_handler(struct ClientHandler *handler)
{
    strtable_destroy(handler->mime_table);
}

bool route_starts_with(const strview_t rootpath, const strview_t suffix, const strview_t route)
{
    if (route.length < rootpath.length + suffix.length) {
        return false;
    }
    return memcmp(route.buf, rootpath.buf, rootpath.length) == 0
        && memcmp(&route.buf[rootpath.length], suffix.buf, suffix.length) == 0;
}

static const char RESPONSE_404_NOT_FOUND[] = "HTTP/1.0 404 Not Found\r\n"
                                             "Content-Type: text/plain\r\n"
                                             "Content-Length: 15\r\n"
                                             "\r\n"
                                             "404 Bad Request";

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

    // just ignore the headers:
    struct HTTPHeader header = {0};
    do {
        char linebuf[1024] = {0};
        size_t line_len = 0;
        e = bytes_recvline(&reader, sizeof(linebuf), linebuf, &line_len);
        if (e.tag != ERROR_NONE) goto on_error;

        if (line_len == 0 || strncmp(linebuf, "\r\n", line_len) == 0) {
            break;
        }

        e = tokenize_header(strview_from_sized((uint8_t *)linebuf, line_len), &header);
        if (e.tag != ERROR_NONE) goto on_error;
    } while (true);

    char path_buf[PATH_MAX] = {0};

    if ((strview_equals(STRVIEW_FROM("/"), request_line.url) || //
         strview_equals(STRVIEW_FROM("/index.html"), request_line.url))
        && (snprintf(path_buf, sizeof(path_buf), "%s/index.html", handler->rootpath.buf), file_exists(path_buf))) {
        strdyn_t file_size_str;
        open_file_and_get_file_size(path_buf, &file_size_str);
        Error_t e1 = send_file_response(conn_fd, get_mime_type(handler, path_buf), file_size_str, path_buf);
        strdyn_free(file_size_str);
        return e1;
    }

    if (strview_equals(STRVIEW_FROM("/favicon.ico"), request_line.url)
        && (printf("true"),
            snprintf(path_buf, sizeof(path_buf), "%s/favicon.ico", handler->rootpath.buf),
            file_exists(path_buf))) {
        strdyn_t file_size_str;
        open_file_and_get_file_size(path_buf, &file_size_str);
        Error_t e1 = send_file_response(conn_fd, get_mime_type(handler, path_buf), file_size_str, path_buf);
        strdyn_free(file_size_str);
        return e1;
    }

    snprintf(
        path_buf,
        sizeof(path_buf),
        "%s%.*s",
        handler->rootpath.buf,
        (int)request_line.url.length,
        request_line.url.buf);
    char real_path_buf[PATH_MAX] = {0};
    const strview_t real_path_view = strview_from_cstr(realpath(path_buf, real_path_buf));

    if ((route_starts_with(handler->rootpath, STRVIEW_FROM("/html/"), real_path_view) || //
         route_starts_with(handler->rootpath, STRVIEW_FROM("/css/"), real_path_view) ||  //
         route_starts_with(handler->rootpath, STRVIEW_FROM("/js/"), real_path_view) ||   //
         route_starts_with(handler->rootpath, STRVIEW_FROM("/images/"), real_path_view))
        && file_exists(real_path_buf)) {
        strdyn_t file_size_str;
        open_file_and_get_file_size(path_buf, &file_size_str);
        Error_t e1 = send_file_response(conn_fd, get_mime_type(handler, real_path_buf), file_size_str, real_path_buf);
        strdyn_free(file_size_str);
        return e1;
    }

    e.tag = ERROR_CUSTOM;
    e.custom_msg = "no matching routes! sending 404";
    e = error_format_location(ERROR_INFO("handle_client"), e);

on_error:
    // don't report any errors. just send 404, possibly a custom 404 if it exists:
    if (snprintf(path_buf, sizeof(path_buf), "%s/html/404.html", (const char *)handler->rootpath.buf),
        file_exists(path_buf)) {
        strdyn_t file_size_str;
        open_file_and_get_file_size(path_buf, &file_size_str);
        Error_t e1 = send_file_response(conn_fd, get_mime_type(handler, path_buf), file_size_str, path_buf);
        strdyn_free(file_size_str);
        if (e1.tag != ERROR_NONE) e = e1;
    }
    else {
        bytes_sendall(conn_fd, sizeof(RESPONSE_404_NOT_FOUND) - 1, RESPONSE_404_NOT_FOUND);
    }
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
