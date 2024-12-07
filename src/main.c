#include "iter_addrinfo.h"

#include <stdio.h>
#include <stdlib.h>

struct print_addrinfo_args {
    const char *hostname;
    int count;
};
void print_addrinfo(void *arg, struct addrinfo *addrinfo, const char *ipver, const char *ipstr)
{
    struct print_addrinfo_args *args = arg; // type pune args
    (void)(addrinfo);                       // mark unused

    if (args->count == 0) {
        printf("IP addresses for %s:\n\n", args->hostname);
    }
    printf("%d. %s: %s\n", args->count++, ipver, ipstr);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "usage: <program> hostname\n");
        return EXIT_FAILURE;
    }

    Error_t error;
    char error_buf[512];

    struct print_addrinfo_args args;
    args.hostname = argv[1];
    args.count = 0;
    error = iter_addrinfo(argv[1], &args, print_addrinfo);

    if (error.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(error, sizeof error_buf, error_buf));
    }

    return EXIT_SUCCESS;
}

/*
    int domain = AF_INET;   // IPv4
    int type = SOCK_STREAM; // TCP socket
    int protocol = 0;       // set to default

    assert(htons(8080) == 0x901f); // asserting the same value as in the video

    int listen_fd = socket(domain, type, protocol);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,   // IPv4
        .sin_port = htons(8080), // in network order
        .sin_addr = 0            // IPv4 address: localhost
    };
    if (bind(listen_fd, (void*)&addr, sizeof(addr)) != 0) {
        perror("bind() failed: ");
        return 1;
    }
    listen(listen_fd, 10);

    int client_fd = accept(listen_fd, 0, 0);

    char buffer[256] = {0};
    recv(client_fd, buffer, 256, 0);

    // GET /file.html ....

    char* f = buffer + 5;
    *strchr(f, ' ') = '\0';

    int opened_fd = open(f, O_RDONLY);

    long* offset = NULL;
    size_t count = 256;
    sendfile(client_fd, opened_fd, offset, count);

    close(opened_fd);
    close(client_fd);
    close(listen_fd);
*/
