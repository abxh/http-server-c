set(NAME 05-tokenize-http-request)

add_executable (${NAME} main.c)

target_link_libraries (${NAME} LINK_PUBLIC lib)
target_include_directories (${NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

set_target_properties(${NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
