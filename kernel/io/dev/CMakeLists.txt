add_library(io_dev OBJECT
    dev.c
    op.c
    res.c
    rp.c
    vol.c
    )

target_include_directories(io_dev PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(io_dev PRIVATE kernel_config)