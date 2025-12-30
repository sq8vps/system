add_library(io_video OBJECT
    output.c
    )

target_include_directories(io_video PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(io_video PRIVATE kernel_config)