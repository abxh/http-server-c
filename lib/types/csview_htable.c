#include "csview_htable.h"

#define NAME               csview_htable
#define KEY_TYPE           csview
#define VALUE_TYPE         csview
#define KEY_IS_EQUAL(a, b) (csview_equals_sv((a), (b)))
#define HASH_FUNCTION(key) (fnvhash_32((uint8_t *)(key).buf, (size_t)(key).size))
#define FUNCTION_DEFINITIONS
#include "fhashtable/fhashtable_template.h"
