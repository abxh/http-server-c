
#include <stdio.h>
#include <stdlib.h>

#include "connection.h"
#include "connection_tcp.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        const char *program_name = (argc == 1) ? argv[0] : "<program>";
        fprintf(stderr, "usage: %s <port>\n", program_name);
        return EXIT_FAILURE;
    }

    const char *hostname = "localhost";
    const char *port = (argc > 1) ? argv[1] : 0;

    char error_buf[512] = {0};
    int client_fd = -1;
    int return_status = EXIT_SUCCESS;

    const Error_t error_open = open_tcp_client(hostname, port, &client_fd);
    if (error_open.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(error_open, sizeof(error_buf), error_buf));
        return_status = EXIT_FAILURE;
    }

    const Error_t error_close = close_socket(client_fd);
    if (error_close.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(error_close, sizeof(error_buf), error_buf));
        return_status = EXIT_FAILURE;
    }

    return return_status;
}
