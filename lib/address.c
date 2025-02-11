#include "address.h"
#include "error.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

Error_t get_peer_address_(const ErrorInfo_t ei, const int fd, socklen_t *out_addr_len, struct sockaddr *out_addr)
{
    RETURN_IF_NULL(ei, out_addr_len);
    RETURN_IF_NULL(ei, out_addr);

    if (getpeername(fd, out_addr, out_addr_len) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t print_address_(const ErrorInfo_t ei, const struct sockaddr *addr, const size_t buf_len, char *out_buf)
{
    RETURN_IF_NULL(ei, out_buf);
    RETURN_IF_NULL(ei, addr);

    if (inet_ntop(addr->sa_family, addr, out_buf, (socklen_t)buf_len) == NULL) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t get_binary_addr_(const ErrorInfo_t ei, const int family, const char *src, void *out_buf)
{
    RETURN_IF_NULL(ei, src);
    RETURN_IF_NULL(ei, out_buf);

    const int status = inet_pton(family, src, out_buf);
    if (status == 0) {
        return error_format_location(
            ei,
            (Error_t){.tag = ERROR_CUSTOM,
                      .custom_msg = "argument src does not contain representating a valid network "
                                    "address in the specified address family."});
    }
    else if (status == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t iter_addrinfo_tcp_(
    const ErrorInfo_t ei,
    const char *hostname,
    const char *port,
    void *func_arg,
    void **func_res,
    void *(*func)(void *arg, const struct addrinfo *addrinfo))
{
    RETURN_IF_NULL(ei, func_arg);
    RETURN_IF_NULL(ei, func);

    if (func_res) {
        *func_res = NULL;
    }

    struct addrinfo hints = {0};
    hints.ai_flags = AF_UNSPEC, hints.ai_socktype = SOCK_STREAM,
    hints.ai_flags = AI_ADDRCONFIG | (hostname == NULL ? AI_PASSIVE : 0);

    struct addrinfo *res;
    int gai_errcode;
    if ((gai_errcode = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        if (gai_errcode == EAI_SYSTEM) {
            return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
        }
        else {
            return error_format_location(ei, (Error_t){.tag = ERROR_GAI, .gai_errcode = gai_errcode});
        }
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
