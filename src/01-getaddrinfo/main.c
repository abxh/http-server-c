#include "address.h"

#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <sys/socket.h>

struct print_iter_func_args {
    const char *hostname;
    int count;
};

void *print_iter_func(void *arg, const struct addrinfo *addrinfo)
{
    struct print_iter_func_args *args = arg; // type pune args

    if (args->count == 0) {
        printf("IP addresses for %s:\n\n", args->hostname);
    }
    char ipstr[INET6_ADDRSTRLEN] = {0};
    const Error_t print_error = print_address(addrinfo->ai_addr, sizeof(ipstr), ipstr);

    if (print_error.tag != ERROR_NONE) {
        char error_buf[64] = {0};
        printf("%s\n", error_stringify(print_error, sizeof(error_buf), error_buf));
    }
    else {
        printf("%d. %s: %s\n", args->count++, addrinfo->ai_family == AF_INET ? "IPv4" : "IPv6", ipstr);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        const char *program_name = (argc == 1) ? argv[0] : "<program>";
        fprintf(stderr, "usage: %s <hostname>\n", program_name);
        return EXIT_FAILURE;
    }

    char error_buf[512] = {0};
    struct print_iter_func_args args = {.hostname = argv[1], .count = 0};

    const char *port = NULL;    // automatically find the port
    void **func_res_ptr = NULL; // do not care about return values

    const Error_t iter_error = iter_addrinfo_tcp(argv[1], port, &args, func_res_ptr, print_iter_func);
    if (iter_error.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(iter_error, sizeof error_buf, error_buf));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
