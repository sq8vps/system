add_library(ke_sys STATIC
    file.c
    mmap.c
    syscall.c
    )

target_include_directories(ke_sys PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})