add_library(libc_syscalls OBJECT
    sysclose.c
    sysexecve.c
    sysfcntl.c
    sysfork.c
    sysfstat.c
    sysgetentropy.c
    sysgetpid.c
    sysgettod.c
    sysisatty.c
    syskill.c
    syslink.c
    syslseek.c
    sysopen.c
    sysread.c
    syssbrk.c
    sysstat.c
    systimes.c
    sysunlink.c
    syswait.c
    syswrite.c
)

target_include_directories(libc_syscalls PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_syscalls PRIVATE libc_includes)