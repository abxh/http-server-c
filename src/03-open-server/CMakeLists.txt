add_executable (03-open-server main.c)

target_link_libraries (03-open-server LINK_PUBLIC lib)
target_include_directories (03-open-server PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

set_target_properties(03-open-server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
