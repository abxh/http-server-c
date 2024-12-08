#pragma once

#include "error.h"

#include <netdb.h>
#include <sys/socket.h>

// note:
// - func_res is nullable.
// - Returning NULL from iter_addrinfo_f is permitted. It continues to iterate if so.

typedef void *(*iter_addrinfo_f)(void *arg, struct addrinfo *addrinfo, const char *ipstr);

Error_t iter_addrinfo_(
    const char *calleename,
    const uint64_t linenr,
    const char *filename,
    const char *hostname,
    void *func_arg,
    void **func_res,
    const iter_addrinfo_f func);

#define iter_addrinfo(...) iter_addrinfo_(__func__, __LINE__, __FILE__, __VA_ARGS__)
