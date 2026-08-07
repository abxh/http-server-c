cmake_minimum_required(VERSION 3.10)
project(http-server-c)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (PROJECT_IS_TOP_LEVEL)
    add_compile_options(-Wall -Wextra -pedantic -ggdb3 -Wmaybe-uninitialized -Wshadow -Wconversion -std=gnu23)

    add_compile_options(-fsanitize=address -fsanitize=undefined)
    add_link_options(-fsanitize=address -fsanitize=undefined)
endif ()

include(FetchContent)
set(EXTERNAL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external")

FetchContent_Declare(
    dsa
    GIT_REPOSITORY https://github.com/abxh/data-structures-c.git
    GIT_TAG v0.1.1
    SOURCE_DIR "${EXTERNAL_DIR}/data-structures-c"
)
FetchContent_MakeAvailable(dsa)
if(NOT TARGET dsa)
    add_library(dsa INTERFACE)
    target_include_directories(dsa INTERFACE ${dsa_SOURCE_DIR})
endif()

add_subdirectory(lib)


if (PROJECT_IS_TOP_LEVEL)
    add_subdirectory(examples/00-minimal)
    add_subdirectory(examples/01-getaddrinfo)
    add_subdirectory(examples/02-open-client)
    add_subdirectory(examples/03-open-server)
    add_subdirectory(examples/04-dummy-http-response)
    add_subdirectory(examples/05-tokenizing-and-assembling)
    add_subdirectory(examples/06-sendfile)
    add_subdirectory(examples/07-static-file-server)
endif()
