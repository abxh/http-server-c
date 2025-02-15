#pragma once

#include "error.h"

#include <stdbool.h>

/**
 * Open a tcp client socket.
 */
Error_t open_tcp_client_(const ErrorInfo_t ei, const char *hostname, const char *port, int *out_client_fd);

/**
 * Open a tcp server socket.
 */
Error_t open_tcp_server_(const ErrorInfo_t ei, const int backlog_size, const char *port, int *out_server_fd);

/**
 * Open a tcp client connection socket from the server.
 */
Error_t open_tcp_client_connection_(const ErrorInfo_t ei, const int server_fd, int *out_conn_fd);

/**
 * Send a stream of bytes.
 */
Error_t
bytes_sendall_(const ErrorInfo_t ei, const int flags, const int conn_fd, const size_t nbytes, const char *inp_buf);

/**
 * Send a file
 */
Error_t bytes_sendfile_(const ErrorInfo_t ei, size_t buf_size, const int conn_fd, const int file_fd);

/**
 * Buffered reader
 */
struct BufferedReader {
    size_t max_msg_len; ///< max message length
    size_t nleft;       ///< number of bytes to read left
    char *msgbuf;       ///< underlying message buffer
    char *curr;         ///< current pointer in message buffer
    int conn_fd;        ///< connection socket file handle
    int recv_flags;     ///< connection flags
};

/**
 * Intiate struct for buffered reading. msgbuf should be able to contain the largest possible message.
 */
void buffered_reader_init_(
    const int recv_flags, struct BufferedReader *reader, const int conn_fd, const size_t max_msg_len, char *msgbuf);

/**
 * Flush the current the stream of bytes.
 */
void buffered_reader_flush(struct BufferedReader *reader);

/**
 * Recvieve some number of bytes from a buffered stream of bytes.
 */
Error_t bytes_recvn_(
    const ErrorInfo_t ei, struct BufferedReader *reader, const size_t nbytes, char *out_buf, size_t *out_nread);

/**
 * Recvieve a line from a buffered stream of bytes.
 */
Error_t bytes_recvline_(
    const ErrorInfo_t ei, struct BufferedReader *reader, const size_t max_len, char *out_buf, size_t *out_len);

#define open_tcp_client(...)              open_tcp_client_(ERROR_INFO("open_tcp_client"), __VA_ARGS__)
#define open_tcp_server(...)              open_tcp_server_(ERROR_INFO("open_tcp_server"), 20, __VA_ARGS__)
#define open_tcp_server_with_backlog(...) open_tcp_server_(ERROR_INFO("open_tcp_server_with_backlog"), __VA_ARGS__)
#define open_tcp_client_connection(...) \
    open_tcp_client_connection_(ERROR_INFO("open_tcp_client_connection"), __VA_ARGS__)
#define bytes_sendall(...)        bytes_sendall_(ERROR_INFO("bytes_sendall"), 0, __VA_ARGS__)
#define bytes_sendfile(...)       bytes_sendfile_(ERROR_INFO("bytes_sendfile"), 4096, __VA_ARGS__)
#define bytes_recvn(...)          bytes_recvn_(ERROR_INFO("bytes_recvn"), __VA_ARGS__)
#define bytes_recvline(...)       bytes_recvline_(ERROR_INFO("bytes_recvline"), __VA_ARGS__)
#define buffered_reader_init(...) buffered_reader_init_(0, __VA_ARGS__)
