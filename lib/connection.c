#include "connection.h"
#include "address.h"

#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

Error_t close_socket_(
    const char *funcname, const char *calleename, const uint64_t linenr, const char *filename, const int fd)
{
    if (close(fd) == -1) {
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
    RETURN_IF_NULL(addrinfo, funcname, calleename, linenr, filename);

    struct sockaddr *addr;
    socklen_t len;
    const int status = connect(
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

Error_t bind_socket_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    const int fd,
    struct addrinfo *addrinfo)
{
    RETURN_IF_NULL(addrinfo, funcname, calleename, linenr, filename);

    // get rid of 'Address already in use' error message.
    const int optval = 1;
    const socklen_t optlen = sizeof(optval);
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen) == -1) {
        Error_t error;
        error_format_location(&error, funcname, calleename, linenr, filename);
        error.tag = ERROR_ERRNO;
        error.errno_num = errno;
        return error;
    }

    if (bind(fd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1) {
        Error_t error;
        error_format_location(&error, funcname, calleename, linenr, filename);
        error.tag = ERROR_ERRNO;
        error.errno_num = errno;
        return error;
    }

    return NO_ERRORS;
}

Error_t listen_socket_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    const int fd,
    const int backlog_size)
{
    if (listen(fd, backlog_size) == -1) {
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
    *out_fd = -1;

    int domain, type, protocol;
    const int fd = socket(
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

struct get_first_successfull_args {
    const char *funcname;
    const char *calleename;
    const uint64_t linenr;
    const char *filename;
    Error_t (*func)(const char *, const char *, const uint64_t, const char *, const int, struct addrinfo *);
    Error_t return_error;
    int *out_fd;
};

void *get_first_successfull(void *arg, struct addrinfo *addrinfo)
{
    struct get_first_successfull_args *args = arg;
    const char *funcname = args->funcname;
    const char *calleename = args->calleename;
    const uint64_t linenr = args->linenr;
    const char *filename = args->filename;
    int *out_fd = args->out_fd;

    Error_t e;

    e = open_generic_socket_(funcname, calleename, linenr, filename, addrinfo, out_fd);
    if (e.tag != ERROR_NONE) {
        return NULL;
    }
    e = args->func(funcname, calleename, linenr, filename, *out_fd, addrinfo);
    if (e.tag == ERROR_NONE) {
        args->return_error = NO_ERRORS;
        return &args->return_error;
    }
    e = close_socket_(funcname, calleename, linenr, filename, *out_fd);
    if (e.tag != ERROR_NONE) {
        args->return_error = e;
        return &args->return_error;
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
    *out_fd = -1;

    struct get_first_successfull_args args = {
        .funcname = funcname,
        .calleename = calleename,
        .linenr = linenr,
        .filename = filename,
        .func = connect_socket_,
        .return_error = NO_ERRORS,
        .out_fd = out_fd,
    };
    Error_t *iter_error;
    Error_t error;

    error = iter_addrinfo_(
        funcname,
        calleename,
        linenr,
        filename,
        SOCK_STREAM,   // TCP
        AI_ADDRCONFIG, // require that client has a valid (IPv4 / IPv6) address
        hostname,
        port,
        &args,
        (void **)&iter_error,
        get_first_successfull);

    if (error.tag != ERROR_NONE) {
        return error;
    }
    else if (iter_error == NULL) {
        error_format_location(&error, funcname, calleename, linenr, filename);
        error.tag = ERROR_CUSTOM;
        error.custom_msg = "unable to connect to any addresses";
        return error;
    }
    else if ((*iter_error).tag != ERROR_NONE) {
        return *iter_error;
    }
    else {
        return NO_ERRORS;
    }
}

Error_t open_server_(
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const int server_type,
    const int backlog_size,
    const char *port,
    int *out_fd)
{
    RETURN_IF_NULL(port, funcname, calleename, linenr, filename);
    RETURN_IF_NULL(out_fd, funcname, calleename, linenr, filename);
    *out_fd = -1;

    struct get_first_successfull_args args = {
        .funcname = funcname,
        .calleename = calleename,
        .linenr = linenr,
        .filename = filename,
        .func = bind_socket_,
        .return_error = NO_ERRORS,
        .out_fd = out_fd,
    };
    Error_t *iter_error;
    Error_t addrinfo_error;

    const char *hostname;
    addrinfo_error = iter_addrinfo_(
        funcname,
        calleename,
        linenr,
        filename,
        server_type,
        AI_ADDRCONFIG | AI_PASSIVE, // require that server has a valid (IPv4 / IPv6) address and fill that in
        hostname = NULL,            // hostname to be filled in
        port,
        &args,
        (void **)&iter_error,
        get_first_successfull);

    if (addrinfo_error.tag != ERROR_NONE) {
        return addrinfo_error;
    }
    else if (iter_error == NULL) {
        error_format_location(&addrinfo_error, funcname, calleename, linenr, filename);
        addrinfo_error.tag = ERROR_CUSTOM;
        addrinfo_error.custom_msg = "unable to bind to any addresses";
        return addrinfo_error;
    }
    else if ((*iter_error).tag != ERROR_NONE) {
        return *iter_error;
    }

    Error_t listen_error = listen_socket_(funcname, calleename, linenr, filename, *out_fd, backlog_size);
    if (listen_error.tag != ERROR_NONE) {
        return addrinfo_error;
    }

    return NO_ERRORS;
}

Error_t open_connection_(
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const int server_fd,
    int *out_fd)
{
    RETURN_IF_NULL(out_fd, funcname, calleename, linenr, filename);
    *out_fd = -1;

    struct sockaddr *addr;
    socklen_t *addr_len_ptr;
    const int fd = accept(
        server_fd,
        addr = NULL, // the address can be retrieved later with getpeername()
        addr_len_ptr = NULL);

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
