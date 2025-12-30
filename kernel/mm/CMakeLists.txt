add_library(mm OBJECT
    dynmap.c
    heap.c
    mm.c
    mmio.c
    palloc.c
    slab.c
    tmem.c
)

target_include_directories(mm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(mm PRIVATE kernel_config)