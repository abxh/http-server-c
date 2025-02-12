file(GLOB LIB "*.c" "types/*.c")
add_library(lib ${LIB})
target_include_directories (lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../external/STC/include)
target_include_directories (lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../external/dsa-c)
