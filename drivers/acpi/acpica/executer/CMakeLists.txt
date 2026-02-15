add_library(executer OBJECT
    exconcat.c
    exconfig.c
    exconvrt.c
    excreate.c
    exdebug.c
    exdump.c
    exfield.c
    exfldio.c
    exmisc.c
    exmutex.c
    exnames.c
    exoparg1.c
    exoparg2.c
    exoparg3.c
    exoparg6.c
    exprep.c
    exregion.c
    exresnte.c
    exresolv.c
    exresop.c
    exserial.c
    exstore.c
    exstoren.c
    exstorob.c
    exsystem.c
    extrace.c
    exutils.c
    )

target_include_directories(executer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include ${DRIVER_API})