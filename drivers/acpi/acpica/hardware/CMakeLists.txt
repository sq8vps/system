add_library(hardware OBJECT
    hwacpi.c
    hwesleep.c
    hwgpe.c
    hwpci.c
    hwregs.c
    hwsleep.c
    hwtimer.c
    hwvalid.c
    hwxface.c
    hwxfsleep.c
    )

target_include_directories(hardware PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${CMAKE_SOURCE_DIR}/../api)