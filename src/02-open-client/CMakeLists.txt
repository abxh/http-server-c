add_executable (02-open-client main.c)

target_link_libraries (02-open-client LINK_PUBLIC lib)
target_include_directories (02-open-client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../lib)

set_target_properties(02-open-client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
