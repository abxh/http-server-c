#pragma once

#include "error.h"

#include <netdb.h>

/**
 * Open a socket, given a addrinfo, and get a file handle controlling the
 * socket.
 */
Error_t open_socket_(const ErrorInfo_t ei, const struct addrinfo *addrinfo, int *out_fd);

/**
 * Close a socket, given the file handle from opening the socket.
 */
Error_t close_socket_(const ErrorInfo_t ei, const int fd);

/**
 * Try connecting with a socket (to a server)
 */
Error_t connect_socket_(const ErrorInfo_t ei, const int fd, const struct addrinfo *addrinfo);

/**
 * Try binding (a server) to a port (a part of addrinfo)
 */
Error_t bind_socket_(const ErrorInfo_t ei, const int fd, const struct addrinfo *addrinfo);

/**
 * Set up port to listen with max incoming connections requests of backlog_size.
 */
Error_t listen_socket_(const ErrorInfo_t ei, const int fd, const int backlog);

#define open_socket(...)    open_socket_(ERROR_INFO("open_socket"), __VA_ARGS__)
#define close_socket(...)   close_socket_(ERROR_INFO("close_socket"), __VA_ARGS__)
#define connect_socket(...) connect_socket_(ERROR_INFO("connect_socket"), __VA_ARGS__)
#define bind_socket(...)    bind_socket_(ERROR_INFO("bind_socket"), __VA_ARGS__)
#define listen_socket(...)  listen_socket_(ERROR_INFO("listen_socket"), __VA_ARGS__)
