add_library(ke_sched STATIC
    idle.c
    sched.c
    sleep.c
    )

target_include_directories(ke_sched PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})