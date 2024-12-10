
#include <stdio.h>
#include <stdlib.h>

#include "address_info.h"
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

    int server_fd, connection_fd;

    const int backlog_size = 1;
    e = open_server_with_custom_backlog_size(backlog_size, port, &server_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    struct sockaddr_storage clientaddr;
    socklen_t clientaddr_size = sizeof(clientaddr);

    e = open_connection_and_get_address(&clientaddr, &clientaddr_size, server_fd, &connection_fd);
    if (e.tag != ERROR_NONE) goto on_error;

    e = print_address(clientaddr.ss_family, (struct sockaddr *)&clientaddr, sizeof(strbuf), strbuf);
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
