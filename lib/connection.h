#pragma once

#include "error.h"

struct addrinfo;

/**
 * Close a socket, given the file handle from opening the socket.
 */
Error_t close_socket_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    int fd);

/**
 * Try connecting with a socket (to a server)
 */
Error_t connect_socket_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    const int fd,
    struct addrinfo *addrinfo);

/**
 * Open a socket, given a addrinfo, and get a file handle abstracting over the socket.
 */
Error_t open_generic_socket_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    struct addrinfo *addrinfo,
    int *out_fd);

/**
 * Open available socket with hostname and nullable(optional) port and connect.
 * Work as a 'client' with respect to the client-server model.
 *
 * This socket can be used to interact with the server.
 */
Error_t open_client_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    const char *hostname,
    const char *port,
    int *out_fd);

/**
 * Open socket with port and listen for incoming connection requests. Prepare to
 * work as a 'server' with respect to the client-server model.
 */
Error_t open_server_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    const char *port,
    const int max_incoming_connection_requests, // usually: 20
    int *out_fd);

/**
 * Wait and accept a single incoming connection request, if any. Return the file
 * handle corresponding to the socket to the connectee.
 *
 * This socket can be used to interact with a single client.
 */
Error_t open_connection_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    const int server_fd,
    int *out_fd);

#define close_socket(...)   close_socket_("close_socket", __func__, __LINE__, __FILE__, __VA_ARGS__)
#define connect_socket(...) connect_socket_("connect_socket", __func__, __LINE__, __FILE__, __VA_ARGS__)
#define open_generic_socket(...) \
    open_generic_socket_("open_generic_socket_", __func__, __LINE__, __FILE__, __VA_ARGS__)
#define open_client(...)     open_client_("open_client", __func__, __LINE__, __FILE__, __VA_ARGS__)
#define open_server(...)     open_server_("open_server", __func__, __LINE__, __FILE__, __VA_ARGS__)
#define open_connection(...) open_connection_("open_connection", __func__, __LINE__, __FILE__, __VA_ARGS__)
