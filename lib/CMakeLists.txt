file(GLOB SOURCES "*.c")
add_library(lib ${SOURCES})

target_include_directories (lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
