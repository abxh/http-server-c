#include "connection.h"
#include "iter_addrinfo.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

Error_t close_socket_(
    const char *funcname, const char *calleename, const uint64_t linenr, const char *filename, int fd)
{
    int status = close(fd);
    if (status == -1) {
        Error_t error;
        error_format_location(&error, funcname, calleename, linenr, filename);
        error.tag = ERROR_ERRNO;
        error.errno_num = errno;
        return error;
    }
    return NO_ERRORS;
}

Error_t connect_socket_(
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const int fd,
    struct addrinfo *addrinfo)
{
    const struct sockaddr *addr;
    socklen_t len;
    int status = connect(
        fd,                        // socket
        addr = addrinfo->ai_addr,  // (IPv4 / IPv6 / ...) address
        len = addrinfo->ai_addrlen // address len
    );
    if (status == -1) {
        Error_t error;
        error_format_location(&error, funcname, calleename, linenr, filename);
        error.tag = ERROR_ERRNO;
        error.errno_num = errno;
        return error;
    }
    return NO_ERRORS;
}

Error_t open_generic_socket_(
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    struct addrinfo *addrinfo,
    int *out_fd)
{
    RETURN_IF_NULL(addrinfo, funcname, calleename, linenr, filename);
    RETURN_IF_NULL(out_fd, funcname, calleename, linenr, filename);

    int domain, type, protocol;
    int fd = socket(
        domain = addrinfo->ai_family,    // IPv4 / IPv6 / ...
        type = addrinfo->ai_socktype,    // TCP / UDP / ...
        protocol = addrinfo->ai_protocol // HTTP / FTP / ...
    );

    if (fd == -1) {
        Error_t error;
        error_format_location(&error, funcname, calleename, linenr, filename);
        error.tag = ERROR_ERRNO;
        error.errno_num = errno;
        return error;
    }

    *out_fd = fd;
    return NO_ERRORS;
}

struct connect_to_first_successfull_args {
    const char *funcname;
    const char *calleename;
    const uint64_t linenr;
    const char *filename;
    Error_t return_value;
};

void *connect_to_first_successfull(void *arg, struct addrinfo *addrinfo, const char *ipstr)
{
    (void)(ipstr);
    struct connect_to_first_successfull_args *args = arg;
    const char *funcname = args->funcname;
    const char *calleename = args->calleename;
    const uint64_t linenr = args->linenr;
    const char *filename = args->filename;

    int fd;
    Error_t e;

    e = open_generic_socket_(funcname, calleename, linenr, filename, addrinfo, &fd);
    if (e.tag != ERROR_NONE) {
        return NULL;
    }
    e = connect_socket_(funcname, calleename, linenr, filename, fd, addrinfo);
    if (e.tag == ERROR_NONE) {
        args->return_value = NO_ERRORS;
        return &args->return_value;
    }
    e = close_socket_(funcname, calleename, linenr, filename, fd);
    if (e.tag != ERROR_NONE) {
        args->return_value = e;
        return &args->return_value;
    }

    return NULL;
}

Error_t open_client_(
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const char *hostname,
    const char *port,
    int *out_fd)
{
    RETURN_IF_NULL(hostname, funcname, calleename, linenr, filename);
    RETURN_IF_NULL(out_fd, funcname, calleename, linenr, filename);

    struct connect_to_first_successfull_args args = {
        .funcname = funcname,
        .calleename = calleename,
        .linenr = linenr,
        .filename = filename,
        .return_value = NO_ERRORS,
    };
    Error_t *iter_error = NULL;
    Error_t error;

    error = iter_addrinfo_(
        funcname,
        calleename,
        linenr,
        filename,
        hostname,
        port,
        &args,
        (void **)&iter_error,
        connect_to_first_successfull);

    if (error.tag != ERROR_NONE) {
        return error;
    }
    else if (iter_error == NULL) {
        error_format_location(&error, funcname, calleename, linenr, filename);
        error.tag = ERROR_CUSTOM;
        error.custom_msg = "no available services with the given hostname";
        return error;
    }
    else if ((*iter_error).tag != ERROR_NONE) {
        return *iter_error;
    }
    else {
        return NO_ERRORS;
    }
}
