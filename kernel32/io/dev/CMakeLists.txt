add_library(io_dev STATIC
    dev.c
    op.c
    res.c
    rp.c
    vol.c
    )

target_include_directories(io_dev PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})