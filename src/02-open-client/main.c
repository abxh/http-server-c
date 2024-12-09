
#include <stdio.h>
#include <stdlib.h>

#include "connection.h"

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: <program> port\n");
        return EXIT_FAILURE;
    }

    Error_t error;
    char error_buf[512];
    int client_fd;

    error = open_client("localhost", argv[1], &client_fd);
    if (error.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(error, sizeof error_buf, error_buf));
    }
    close_socket(client_fd);

    return EXIT_SUCCESS;
}
