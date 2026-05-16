add_library(ke_core OBJECT
    dpc.c
    mutex.c
    panic.c
    )

target_include_directories(ke_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(ke_core PRIVATE kernel_config)