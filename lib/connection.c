#include "connection.h"

#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

Error_t open_socket_(const ErrorInfo_t ei, const struct addrinfo *addrinfo, int *out_fd)
{
    RETURN_IF_NULL(ei, out_fd);

    if ((*out_fd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol)) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t close_socket_(const ErrorInfo_t ei, const int fd)
{
    if ((close(fd)) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t connect_socket_(const ErrorInfo_t ei, const int fd, const struct addrinfo *addrinfo)
{
    RETURN_IF_NULL(ei, addrinfo);

    if (connect(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t bind_socket_(const ErrorInfo_t ei, const int fd, const struct addrinfo *addrinfo)
{
    RETURN_IF_NULL(ei, addrinfo);

    // get rid of 'Address already in use' error message.
    const int optval = 1;
    const socklen_t optlen = sizeof(optval);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }

    if (bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen)) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t listen_socket_(const ErrorInfo_t ei, const int fd, const int backlog)
{
    if (listen(fd, backlog) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}
