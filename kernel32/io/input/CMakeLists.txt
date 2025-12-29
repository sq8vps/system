add_library(io_input STATIC
    input.c
    kbd.c
    )

target_include_directories(io_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})