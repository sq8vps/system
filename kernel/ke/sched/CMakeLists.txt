add_library(ke_sched OBJECT
    cfs.c
    fcfs.c
    idle.c
    rr.c
    sched.c
    sleep.c
    )

target_include_directories(ke_sched PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ke_sched PRIVATE kernel_config)