#include "iter_addrinfo.h"

#include <stdio.h>
#include <stdlib.h>

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
    if (argc != 2) {
        fprintf(stderr, "usage: <program> hostname\n");
        return EXIT_FAILURE;
    }

    Error_t error;
    char error_buf[512];

    struct print_addrinfo_args args;
    args.hostname = argv[1];
    args.count = 0;
    error = iter_addrinfo(argv[1], &args, NULL, print_addrinfo);

    if (error.tag != ERROR_NONE) {
        printf("%s\n", error_stringify(error, sizeof error_buf, error_buf));
    }

    return EXIT_SUCCESS;
}
