add_library(libc_machine_i686 STATIC
    memchr.S
    memcmp.S
    memcpy.S
    memmove.S
    memset.S
    setjmp.S
    strchr.S
    strlen.S
)
target_include_directories(libc_machine_i686 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/machine
)