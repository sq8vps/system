add_library(ddk OBJECT
    disk.c
    fs.c
    stor.c
    tty.c
)

target_include_directories(ddk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ddk PRIVATE kernel_config)