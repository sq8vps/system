add_library(tables OBJECT
    tbdata.c
    tbfadt.c
    tbfind.c
    tbinstal.c
    tbprint.c
    tbutils.c
    tbxface.c
    tbxfload.c
    tbxfroot.c
    )

target_include_directories(tables PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${DRIVER_API})