#include "strdyn.h"

#include "../error.h"

#include <assert.h>
#include <string.h>

int main()
{
    Error_t e;

    strdyn_t s = NULL;

    e = strdyn_empty(&s);
    assert(e.tag == ERROR_NONE);
    assert(strdyn_length(s) == 0);
    printf("%s", s);

    e = strdyn_append(&s, "hello");
    assert(e.tag == ERROR_NONE);
    assert(strdyn_length(s) == sizeof("hello") - 1);
    printf("%s", s);

    strdyn_clear(s);
    assert(strdyn_length(s) == 0);
    assert(strcmp(s, "") == 0);
    printf("%s", s);

    e = strdyn_append(&s, " world");
    assert(e.tag == ERROR_NONE);
    assert(strdyn_length(s) == sizeof(" world") - 1);
    printf("%s", s);

    e = strdyn_reserve(&s, 32);
    assert(e.tag == ERROR_NONE);
    assert(strdyn_length(s) == sizeof(" world") - 1);
    printf("%s", s);

    e = strdyn_reserve(&s, 1);
    assert(e.tag == ERROR_NONE);
    assert(strdyn_length(s) == sizeof(" world") - 1);
    printf("%s", s);

    e = strdyn_append_fmt(&s, ", %s\n", "and hello void.");
    assert(e.tag == ERROR_NONE);
    assert(strdyn_length(s) == sizeof(" world, and hello void.\n") - 1);
    printf("%s", s);

    strdyn_free(s);
}
