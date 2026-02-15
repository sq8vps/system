add_library(parser OBJECT
    psargs.c
    psloop.c
    psobject.c
    psopcode.c
    psopinfo.c
    psparse.c
    psscope.c
    pstree.c
    psutils.c
    pswalk.c
    psxface.c
    )

target_include_directories(parser PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${DRIVER_API})