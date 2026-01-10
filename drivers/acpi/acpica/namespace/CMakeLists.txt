add_library(namespace OBJECT
    nsaccess.c
    nsalloc.c
    nsarguments.c
    nsconvert.c
    nsdump.c
    nsdumpdv.c
    nseval.c
    nsinit.c
    nsload.c
    nsnames.c
    nsobject.c
    nsparse.c
    nspredef.c
    nsprepkg.c
    nsrepair.c
    nsrepair2.c
    nssearch.c
    nsutils.c
    nswalk.c
    nsxfeval.c
    nsxfname.c
    nsxfobj.c
    )

target_include_directories(namespace PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_SOURCE_DIR}/../api)