#include "connection_tcp.h"
#include "address.h"
#include "connection.h"

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h>

// based on:
// https://beej.us/guide/bgnet/
// https://csapp.cs.cmu.edu/3e/ics3/code/src/csapp.c

struct get_first_successfull_args {
    const ErrorInfo_t ei;
    Error_t (*func)(const ErrorInfo_t ei, const int, const struct addrinfo *);
    Error_t return_error;
    int *out_fd;
};

static void *get_first_successfull(void *arg, const struct addrinfo *addrinfo)
{
    struct get_first_successfull_args *args = arg;

    const Error_t open_error = open_socket_(args->ei, addrinfo, args->out_fd);
    if (open_error.tag != ERROR_NONE) {
        return NULL;
    }
    const Error_t func_error = args->func(args->ei, *args->out_fd, addrinfo);
    if (func_error.tag == ERROR_NONE) {
        args->return_error = NO_ERRORS;
        return &args->return_error;
    }
    const Error_t close_error = close_socket_(args->ei, *args->out_fd);
    if (close_error.tag != ERROR_NONE) {
        args->return_error = close_error;
        return &args->return_error;
    }
    return NULL;
}

Error_t open_tcp_client_(const ErrorInfo_t ei, const char *hostname, const char *port, int *out_client_fd)
{
    RETURN_IF_NULL(ei, out_client_fd);
    *out_client_fd = -1;
    RETURN_IF_NULL(ei, hostname);

    struct get_first_successfull_args func_args = {
        .ei = ei,
        .func = connect_socket_,
        .return_error = NO_ERRORS,
        .out_fd = out_client_fd,
    };
    void *iter_error = NULL;
    const Error_t iter_addrinfo_error =
        iter_addrinfo_tcp_(ei, hostname, port, &func_args, &iter_error, get_first_successfull);
    if (iter_addrinfo_error.tag != ERROR_NONE) {
        return iter_addrinfo_error;
    }
    else if (iter_error == NULL) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "unable to connect to any addresses"});
    }
    else if ((*(const Error_t *)iter_error).tag != ERROR_NONE) {
        return *(const Error_t *)iter_error;
    }
    else {
        return NO_ERRORS;
    }
}

Error_t open_tcp_server_(const ErrorInfo_t ei, const int backlog_size, const char *port, int *out_server_fd)
{
    RETURN_IF_NULL(ei, out_server_fd);
    *out_server_fd = -1;

    struct get_first_successfull_args func_args = {
        .ei = ei,
        .func = bind_socket_,
        .return_error = NO_ERRORS,
        .out_fd = out_server_fd,
    };
    const char *hostname = NULL; // FILL in details of the current host

    void *iter_error = NULL;
    const Error_t iter_addrinfo_error =
        iter_addrinfo_tcp_(ei, hostname, port, &func_args, &iter_error, get_first_successfull);
    if (iter_addrinfo_error.tag != ERROR_NONE) {
        return iter_addrinfo_error;
    }
    else if (iter_error == NULL) {
        return error_format_location(
            ei, (Error_t){.tag = ERROR_CUSTOM, .custom_msg = "unable to connect to any addresses"});
    }
    else if ((*(const Error_t *)iter_error).tag != ERROR_NONE) {
        return *(const Error_t *)iter_error;
    }

    const Error_t listen_error = listen_socket_(ei, *out_server_fd, backlog_size);
    if (listen_error.tag != ERROR_NONE) {
        return listen_error;
    }
    else {
        return NO_ERRORS;
    }
}

Error_t open_tcp_client_connection_(const ErrorInfo_t ei, const int server_fd, int *out_conn_fd)
{
    RETURN_IF_NULL(ei, out_conn_fd);

    if ((*out_conn_fd = accept(server_fd, NULL, NULL)) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

Error_t
bytes_sendall_(const ErrorInfo_t ei, const int flags, const int conn_fd, const size_t nbytes, const char *inp_buf)
{
    RETURN_IF_NULL(ei, inp_buf);
    if (nbytes == 0) return NO_ERRORS;

    const char *ptr = inp_buf;
    size_t nleft = nbytes;

    do {
        const ssize_t retval = send(conn_fd, ptr, nleft, flags);
        if (retval < 0) {
            if (errno == EINTR) {
                // the send() was interrupted by a signal before it sent any data.
                ptr = inp_buf;
                continue;
            }
            else if (retval == -1) {
                return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
            }
            else {
                assert(false);
            }
        }
        nleft -= (size_t)retval;
        ptr += (size_t)retval;
    } while (nleft > 0);

    return NO_ERRORS;
}

/**
 * Send a file
 */
Error_t bytes_sendfile_(const ErrorInfo_t ei, const int conn_fd, const int file_fd, size_t max_file_size)
{
    if (sendfile(conn_fd, file_fd, NULL, max_file_size) == -1) {
        return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
    }
    return NO_ERRORS;
}

void buffered_reader_init_(
    const int recv_flags, struct BufferedReader *reader, const int conn_fd, const size_t max_msg_len, char *msgbuf)
{
    assert(reader);
    assert(msgbuf);

    reader->max_msg_len = max_msg_len;
    reader->msgbuf = reader->curr = &msgbuf[0];
    reader->conn_fd = conn_fd;
    reader->recv_flags = recv_flags;
    reader->nleft = 0;
}

void buffered_reader_flush(struct BufferedReader *reader)
{
    reader->nleft = 0;
}

static Error_t bytes_recv_unbuffered_(
    const ErrorInfo_t ei,
    const int flags,
    const int conn_fd,
    const size_t max_msg_len,
    char *out_msgbuf,
    size_t *out_nbytes)
{
    assert(max_msg_len != 0);

    char *ptr = out_msgbuf;
    do {
        const ssize_t retval = recv(conn_fd, ptr, max_msg_len, flags);
        if (retval < 0) {
            if (errno == EINTR) {
                // the recv() was interrupted by a signal before it recieved any data.
                ptr = out_msgbuf;
                continue;
            }
            else if (retval == -1) {
                return error_format_location(ei, (Error_t){.tag = ERROR_ERRNO, .errno_num = errno});
            }
            else {
                assert(false);
            }
        }
        *out_nbytes = (size_t)retval;
    } while (false);

    return NO_ERRORS;
}

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

static Error_t bytes_recv_buffered(
    const ErrorInfo_t ei, struct BufferedReader *reader, const size_t nrequested, char *out_buf, size_t *out_nread)
{
    assert(nrequested != 0);

    const bool should_flush = reader->nleft == 0;
    if (should_flush) {
        reader->curr = &reader->msgbuf[0];
        const Error_t error = bytes_recv_unbuffered_(
            ei, reader->recv_flags, reader->conn_fd, reader->max_msg_len, reader->msgbuf, &reader->nleft);
        if (error.tag != ERROR_NONE) {
            return error;
        }
    }
    else if (reader->nleft == 0) {
        // EOF
        *out_nread = 0;
        return NO_ERRORS;
    }

    const size_t cnt_read = MIN(reader->nleft, nrequested);
    memcpy(out_buf, reader->curr, cnt_read);

    reader->curr += cnt_read;
    reader->nleft -= cnt_read;

    *out_nread = cnt_read;
    return NO_ERRORS;
}

Error_t
bytes_recvn_(const ErrorInfo_t ei, struct BufferedReader *reader, const size_t nbytes, char *out_buf, size_t *out_nread)
{
    size_t nleft = nbytes;
    while (nleft > 0) {
        size_t nread = 0;
        const Error_t error = bytes_recv_buffered(ei, reader, nleft, out_buf, &nread);
        if (error.tag != ERROR_NONE) {
            return error;
        }
        else if (nread == 0) {
            // EOF
            break;
        }
        nleft -= nread;
        out_buf += nread;
    };

    *out_nread = nbytes - nleft;
    return NO_ERRORS;
}

Error_t bytes_recvline_(
    const ErrorInfo_t ei, struct BufferedReader *reader, const size_t max_len, char *out_buf, size_t *out_len)
{
    RETURN_IF_NULL(ei, reader);
    RETURN_IF_NULL(ei, out_buf);
    RETURN_IF_NULL(ei, out_len);

    char c = '\0';
    size_t len = 1;
    for (; len < max_len; len++) {
        size_t nread = 0;
        const Error_t error = bytes_recv_buffered(ei, reader, 1, &c, &nread);
        if (error.tag != ERROR_NONE) {
            return error;
        }
        else if (nread == 0) {
            // EOF
            break;
        }
        else {
            *out_buf++ = c;
            if (c == '\n') {
                len++;
                break;
            }
        }
    }
    *out_buf = '\0';
    *out_len = len - 1;
    return NO_ERRORS;
}
