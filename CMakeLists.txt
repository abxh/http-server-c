cmake_minimum_required(VERSION 2.8)
project(http-server-c)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wshadow -Wconversion -pedantic -ggdb3 -Wmaybe-uninitialized -std=gnu11)

add_compile_options(-fsanitize=address -fsanitize=undefined)
add_link_options(-fsanitize=address -fsanitize=undefined)

add_subdirectory(lib)
add_subdirectory(src/00-minimal)
add_subdirectory(src/01-getaddrinfo)
add_subdirectory(src/02-open-client)
