add_library(libc_posix OBJECT
    execv.c
    fnmatch.c
    ftw.c
    glob.c
    posix_spawn.c
    closedir.c
    collcmp.c
    creat.c
    execl.c
    execve.c
    readdir_r.c
    regcomp.c
    regexec.c
    sleep.c
    wordfree.c
    _isatty.c
    collate.c
    dirfd.c
    execle.c
    execvp.c
    opendir.c
    regerror.c
    regfree.c
    scandir.c
    telldir.c
    wordexp.c
    execlp.c
    isatty.c
    popen.c
    readdir.c
    rewinddir.c
    seekdir.c
    usleep.c
)

target_include_directories(libc_posix PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_posix PRIVATE libc_includes)