
#include "strview.h"

#include <string.h>

bool strview_equals(const strview lhs, const strview rhs)
{
    if (lhs.size != rhs.size) {
        return false;
    };
    return memcmp(lhs.buf, rhs.buf, lhs.size) == 0;
}
