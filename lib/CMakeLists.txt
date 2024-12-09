file(GLOB LIB "*.c")
add_library(lib ${LIB})

target_include_directories (lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
