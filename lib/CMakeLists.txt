file(GLOB LIB
    "*.c"
    "types/strview.c"
    "types/strtable.c"
    "types/strdyn.c"
)

add_library(lib ${LIB})

target_include_directories(lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../external
)

target_compile_options(lib PRIVATE
    -Wall
    -Wextra
    -Wshadow
    -Wconversion
    -pedantic
    -Wmaybe-uninitialized
)
