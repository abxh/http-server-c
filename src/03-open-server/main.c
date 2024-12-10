
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "address.h"
#include "connection.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        const char *program_name = (argc == 1) ? argv[0] : "<program>";
        fprintf(stderr, "usage: %s <port>\n", program_name);
        return EXIT_FAILURE;
    }
    const char *port = (argc > 1) ? argv[1] : 0;

    Error_t e;
    int return_status = EXIT_SUCCESS;
    char strbuf[512] = {0};

    int server_fd = -1;
    int conn_fd = -1;

    struct sockaddr_storage clientaddr;
    socklen_t clientaddr_size = sizeof(clientaddr);
    struct sockaddr *clientaddr_ptr = (struct sockaddr *)&clientaddr;

    const int backlog = 1;
    e = open_tcp_server_with_backlog(backlog, port, &server_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    e = open_connection(server_fd, &conn_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    e = get_peer_address(conn_fd, &clientaddr_size, clientaddr_ptr);
    if (e.tag != ERROR_NONE) goto on_error;

    e = print_address(clientaddr.ss_family, clientaddr_ptr, sizeof(strbuf), strbuf);
    if (e.tag != ERROR_NONE) goto on_error;
    printf("In server - connected client address: %s\n", strbuf);

on_error:
    if (e.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(e, sizeof(strbuf), strbuf));
        return_status = EXIT_FAILURE;
    }
    const Error_t error_close = close_socket(server_fd);
    if (error_close.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(error_close, sizeof(strbuf), strbuf));
        return_status = EXIT_FAILURE;
    }

    return return_status;
}
