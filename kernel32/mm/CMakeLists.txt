add_library(mm STATIC
    dynmap.c
    heap.c
    mm.c
    mmio.c
    palloc.c
    slab.c
    tmem.c
)

target_include_directories(mm PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})