add_library(io_input OBJECT
    input.c
    kbd.c
    )

target_include_directories(io_input PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(io_input PRIVATE kernel_config)