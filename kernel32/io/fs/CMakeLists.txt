add_library(io_fs OBJECT
    devfs.c
    fs.c
    taskfs.c
    vfs.c
    )

target_include_directories(io_fs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(io_fs PRIVATE kernel_config)