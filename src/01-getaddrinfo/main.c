#include "iter_addrinfo.h"

#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <sys/socket.h>

struct print_addrinfo_args {
    const char *hostname;
    int count;
};

void *print_addrinfo(void *arg, struct addrinfo *addrinfo, const char *ipstr)
{
    struct print_addrinfo_args *args = arg; // type pune args
    (void)(ipstr);                          // mark unused

    if (args->count == 0) {
        printf("IP addresses for %s:\n\n", args->hostname);
    }
    printf("%d. %s: %s\n", args->count++, addrinfo->ai_family == AF_INET ? "IPv4" : "IPv6", ipstr);

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        const char *program_name = (argc == 1) ? argv[0] : "<program>";
        fprintf(stderr, "usage: %s <port>\n", program_name);
        return EXIT_FAILURE;
    }

    char error_buf[512] = {0};
    struct print_addrinfo_args args = {.hostname = argv[1], .count = 0};

    const char *port;
    void **func_res_ptr;
    const Error_t iter_error = iter_addrinfo(
        argv[1],
        port = NULL,         // automatically find the port
        &args,
        func_res_ptr = NULL, // throw the return value
        print_addrinfo);
    if (iter_error.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(iter_error, sizeof error_buf, error_buf));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
