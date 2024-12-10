#include "address_info.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

Error_t print_address_(
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const int addr_family,
    const struct sockaddr *addr,
    const socklen_t out_buf_len,
    char *out_buf)
{
    RETURN_IF_NULL(addr, funcname, calleename, linenr, filename);
    RETURN_IF_NULL(out_buf, funcname, calleename, linenr, filename);

    if (inet_ntop(addr_family, addr, out_buf, out_buf_len) == NULL) {
        Error_t error;
        error.tag = ERROR_ERRNO;
        error.errno_num = errno;
        error_format_location(&error, funcname, calleename, linenr, filename);
        return error;
    }

    return NO_ERRORS;
}

Error_t iter_addrinfo_(
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const int socktype,
    const int flags,
    const char *hostname,
    const char *port,
    void *func_arg,
    void **func_res,
    const iter_addrinfo_f func)
{
    RETURN_IF_NULL(func_arg, funcname, calleename, linenr, filename);
    RETURN_IF_NULL(func, funcname, calleename, linenr, filename);

    if (func_res) {
        *func_res = NULL;
    }

    struct addrinfo hints = {0};  // zero out struct addrinfo
    hints.ai_family = AF_UNSPEC;  // IPv4 / IPv6 / ...; not specified
    hints.ai_socktype = socktype; // TCP / UDP
    hints.ai_flags = flags;       // extra configuration

    struct addrinfo *res;
    int gai_errcode;
    const char *service;

    gai_errcode = getaddrinfo(
        hostname,       // IPv4 / IPv6 / domain name
        service = port, // port
        &hints,         // family: unspecified, socktype: TCP
        &res            // <output>
    );

    if (gai_errcode != 0) {
        Error_t error;
        error_format_location(&error, funcname, calleename, linenr, filename);
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
        void *func_res_val = func(func_arg, p);
        if (func_res != NULL && func_res_val != NULL) {
            *func_res = func_res_val;
            break;
        }
    }
    freeaddrinfo(res);

    return NO_ERRORS;
}
