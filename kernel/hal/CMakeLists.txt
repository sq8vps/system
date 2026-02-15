add_subdirectory(${CMAKE_SYSTEM_PROCESSOR})

add_library(hal OBJECT
    cpu.c
    hal.c
    interrupt.c
    time.c
)

target_include_directories(hal PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(hal PRIVATE kernel_config)