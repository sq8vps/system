add_library(io_video STATIC
    output.c
    )

target_include_directories(io_video PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})