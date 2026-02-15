add_library(resources OBJECT
    rsaddr.c
    rscalc.c
    rscreate.c
    rsdumpinfo.c
    rsinfo.c
    rsio.c
    rsirq.c
    rslist.c
    rsmemory.c
    rsmisc.c
    rsserial.c
    rsutils.c
    rsxface.c
    )

target_include_directories(resources PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${DRIVER_API})