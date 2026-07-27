#include "strtable.h"

#include <data-structures-c/fhashtable/fnvhash.h>

#define NAME               strtable
#define KEY_TYPE           struct strview
#define VALUE_TYPE         struct strview
#define KEY_IS_EQUAL(a, b) (strview_equals((a), (b)))
#define HASH_FUNCTION(key) (fnvhash_32((uint8_t *)(key).buf, (size_t)(key).size))
#define FUNCTION_DEFINITIONS
#include <data-structures-c/fhashtable/fhashtable_template.h>
