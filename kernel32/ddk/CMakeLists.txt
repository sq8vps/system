add_library(ddk STATIC
    disk.c
    fs.c
    stor.c
    tty.c
)

target_include_directories(ddk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)