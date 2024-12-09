#pragma once

#include "error.h"

struct addrinfo;

typedef void *(*iter_addrinfo_f)(void *arg, struct addrinfo *addrinfo, const char *ipstr);

/**
 * Iterate over addrinfo's, given a hostname and a nullable (optional) port.
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
    // args:
    const char *hostname,
    const char *port,
    void *func_arg,
    void **func_res,
    const iter_addrinfo_f func);

#define iter_addrinfo(...) iter_addrinfo_("iter_addrinfo", __func__, __LINE__, __FILE__, __VA_ARGS__)
