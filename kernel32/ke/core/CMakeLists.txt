add_library(ke_core STATIC
    dpc.c
    mutex.c
    panic.c
    )

target_include_directories(ke_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})