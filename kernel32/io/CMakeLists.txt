add_subdirectory(dev)
add_subdirectory(fs)
add_subdirectory(input)
add_subdirectory(log)
add_subdirectory(video)

add_library(io STATIC
    initrd.c
    )

target_include_directories(io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(io PUBLIC
    io_dev
    io_fs
    io_input
    io_log
    io_video
)