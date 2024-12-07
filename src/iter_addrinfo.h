#pragma once

#include "error.h"

struct addrinfo;

// TODO:
// - remove ipver char*. users can use addrinfo->ai_family
// - make a way to get the return value of func.
typedef void (*iter_addrinfo_f)(void *arg, struct addrinfo *addrinfo, const char *ipver, const char *ipstr);

Error_t iter_addrinfo_(const char *calleename, const uint64_t linenr, const char *filename, const char *hostname,
                       void *func_arg, const iter_addrinfo_f func);

#define iter_addrinfo(...) iter_addrinfo_(__func__, __LINE__, __FILE__, __VA_ARGS__)
