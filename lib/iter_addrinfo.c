#include "iter_addrinfo.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

Error_t iter_addrinfo_(
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const char *hostname,
    void *func_arg,
    void **func_res,
    const iter_addrinfo_f func)
{
    RETURN_IF_NULL(hostname, __func__, calleename, linenr, filename);
    RETURN_IF_NULL(func_arg, __func__, calleename, linenr, filename);
    RETURN_IF_NULL(func, __func__, calleename, linenr, filename);

    struct addrinfo hints = {0}; // zero out struct addrinfo
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res;
    int gai_errcode;
    gai_errcode = getaddrinfo(hostname, NULL, &hints, &res);

    if (gai_errcode != 0) {
        Error_t error;
        error_format_location(&error, __func__, calleename, linenr, filename);
        if (gai_errcode == EAI_SYSTEM) {
            error.tag = ERROR_ERRNO;
            error.errno_num = errno;
        }
        else {
            error.tag = ERROR_GAI;
            error.gai_errcode = gai_errcode;
        }
        return error;
    }

    for (struct addrinfo *p = res; p != NULL; p = p->ai_next) {
        void *addr;

        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4;
            ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
        }
        else if (p->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6;
            ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
        }
        else {
            Error_t error;
            error.tag = ERROR_UNHANDLED;
            error_format_location(&error, __func__, calleename, linenr, filename);
            return error;
        }

        char ipstr[INET6_ADDRSTRLEN];
        if (inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr) == NULL) {
            Error_t error;
            error.tag = ERROR_ERRNO;
            error.errno_num = errno;
            error_format_location(&error, __func__, calleename, linenr, filename);
            return error;
        }

        void *func_res_val = func(func_arg, p, ipstr);
        if (func_res != NULL) {
            *func_res = func_res_val;
            break;
        }
    }
    freeaddrinfo(res);

    return NO_ERRORS;
}
