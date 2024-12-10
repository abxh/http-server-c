#pragma once

#include "error.h"

#include <netdb.h>

typedef void *(*iter_addrinfo_f)(void *arg, struct addrinfo *addrinfo);

/**
 * Print the address.
 *
 * Note:
 * - addrinfo->ai_family can be used to get addr_family.
 * - addrinfo->ai_addr can be used to get addr.
 * - out_buf_len should at least be INET6_ADDRSTRLEN
 */
Error_t print_address_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args:
    const int addr_family, // AF_INET (IPv4), AF_INET6 (IPv6)
    const struct sockaddr *addr,
    const socklen_t out_buf_len,
    char *out_buf);

/**
 * Iterate over addrinfo's, given a nullable hostname and port.
 *
 * Note:
 * - Setting func_res to NULL, ignores the function's return value.
 * - Returning NULL from iter_addrinfo_f is permitted. It continues to iterate if so.
 */
Error_t iter_addrinfo_(
    // error info:
    const char *funcname,
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    // args with defaults:
    const int socktype,
    const int flags,
    // args:
    const char *hostname,
    const char *port,
    void *func_arg,
    void **func_res,
    const iter_addrinfo_f func);

#define iter_addrinfo(...) \
    iter_addrinfo_("iter_addrinfo", __func__, __LINE__, __FILE__, SOCK_STREAM, AI_ADDRCONFIG, __VA_ARGS__)

#define print_address(...) print_address_("print_address", __func__, __LINE__, __FILE__, __VA_ARGS__)
