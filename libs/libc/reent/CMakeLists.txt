add_library(libc_reent OBJECT
    closer.c
    execr.c
    fcntlr.c
#    fstat64r.c
    fstatr.c
    getentropyr.c
    getreent.c
    gettimeofdayr.c
    impure.c
    isattyr.c
    linkr.c
#    lseek64r.c
    lseekr.c
    mkdirr.c
#    open64r.c
    openr.c
    readr.c
    reent.c
    renamer.c
    sbrkr.c
    signalr.c
#    stat64r.c
    statr.c
    timesr.c
    unlinkr.c
    writer.c
)

target_include_directories(libc_reent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_reent PRIVATE libc_includes)