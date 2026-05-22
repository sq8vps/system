add_library(libc_misc OBJECT
    __dprintf.c
    ffs.c
    fini.c
    init.c
    lock.c
    unctrl.c
)

target_include_directories(libc_misc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_misc PRIVATE libc_includes)