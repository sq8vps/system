add_library(ex_kdrv STATIC
    fsdrv.c
    kdrv.c
    
    kdrvid.cpp
    )

target_include_directories(ex_kdrv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})