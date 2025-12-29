add_library(hal STATIC
    cpu.c
    hal.c
    interrupt.c
    time.c
)

target_include_directories(hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(i686)