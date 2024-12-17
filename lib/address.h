#pragma once

#include "error_utils.h"

#include <netdb.h>

/**
 * Get the address a socket is configured to.
 */
Error_t get_peer_address_(const ErrorInfo_t ei, const int fd, socklen_t *out_addr_len, struct sockaddr *out_addr);

/**
 * Print the address.
 */
Error_t print_address_(const ErrorInfo_t ei, const struct sockaddr *addr, const size_t buf_len, char *out_buf);

/**
 * Get an address in binary encoding.
 */
Error_t get_binary_addr_(const ErrorInfo_t ei, const int family, const char *src, void *out_buf);

/**
 * Iterate over possible addresses supporting TCP, given a nullable hostname and non-nullable port.
 *
 * Note:
 * - Setting func_res to NULL, ignores the function's return value.
 * - Returning NULL from func is permitted. It continues to iterate if so.
 */
Error_t iter_addrinfo_tcp_(
    const ErrorInfo_t ei,
    const char *hostname,
    const char *port,
    void *func_arg,
    void **func_res,
    void *(*func)(void *arg, const struct addrinfo *addrinfo));

#define get_peer_address(...)       get_peer_address_(ERROR_INFO("get_peer_address"), __VA_ARGS__)
#define conv_to_binary_address(...) conv_to_binary_address_(ERROR_INFO("conv_to_binary_address"), __VA_ARGS__)
#define print_address(...)          print_address_(ERROR_INFO("print_address"), __VA_ARGS__)
#define iter_addrinfo_tcp(...)      iter_addrinfo_tcp_(ERROR_INFO("iter_addrinfo_tcp"), __VA_ARGS__)
