#pragma once

#include "strview.h"

#define NAME       strtable
#define KEY_TYPE   struct strview
#define VALUE_TYPE struct strview
#define KEY_IS_EQUAL
#define HASH_FUNCTION
#define TYPE_DEFINITIONS
#include <data-structures-c/fhashtable/fhashtable_template.h>

typedef struct strtable strtable;
