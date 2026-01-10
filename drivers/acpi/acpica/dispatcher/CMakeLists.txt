add_library(dispatcher OBJECT
    dsargs.c
    dscontrol.c
    dsdebug.c
    dsfield.c
    dsinit.c
    dsmethod.c
    dsmthdat.c
    dsobject.c
    dsopcode.c
    dspkginit.c
    dsutils.c
    dswexec.c
    dswload.c
    dswload2.c
    dswscope.c
    dswstate.c
    )

target_include_directories(dispatcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_SOURCE_DIR}/../api)