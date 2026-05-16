add_subdirectory(dev)
add_subdirectory(fs)
add_subdirectory(input)
add_subdirectory(log)
add_subdirectory(video)

add_library(io OBJECT
    initrd.c
    )

target_include_directories(io PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(io PUBLIC
    io_dev $<TARGET_OBJECTS:io_dev>
    io_fs $<TARGET_OBJECTS:io_fs>
    io_input $<TARGET_OBJECTS:io_input>
    io_log $<TARGET_OBJECTS:io_log>
    io_video $<TARGET_OBJECTS:io_video>
)
target_link_libraries(io PRIVATE kernel_config)