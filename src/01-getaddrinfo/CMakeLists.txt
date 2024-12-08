add_executable (01-getaddrinfo main.c)

target_link_libraries (01-getaddrinfo LINK_PUBLIC lib)
target_include_directories (01-getaddrinfo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

set_target_properties(01-getaddrinfo PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin)
