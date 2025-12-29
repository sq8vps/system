add_library(io_fs STATIC
    devfs.c
    fs.c
    taskfs.c
    vfs.c
    )

target_include_directories(io_fs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})