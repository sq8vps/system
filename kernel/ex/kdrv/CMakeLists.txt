add_library(ex_kdrv OBJECT
    fsdrv.c
    kdrv.c
    
    kdrvid.cpp
    )

target_include_directories(ex_kdrv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(ex_kdrv PRIVATE kernel_config)