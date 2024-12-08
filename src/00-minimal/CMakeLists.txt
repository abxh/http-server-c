add_executable (00-minimal main.c)

target_link_libraries (00-minimal LINK_PUBLIC lib)
target_include_directories (00-minimal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

set_target_properties(00-minimal PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../bin)
