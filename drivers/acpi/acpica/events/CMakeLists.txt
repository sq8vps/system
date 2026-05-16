add_library(events OBJECT
    evevent.c
    evglock.c
    evgpe.c
    evgpeblk.c
    evgpeinit.c
    evgpeutil.c
    evhandler.c
    evmisc.c
    evregion.c
    evrgnini.c
    evsci.c
    evxface.c
    evxfevnt.c
    evxfgpe.c
    evxfregn.c
    )

target_include_directories(events PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${NABLA_DRIVER_API})