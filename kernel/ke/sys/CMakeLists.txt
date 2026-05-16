add_library(ke_sys OBJECT
    syscall.c
    )

target_include_directories(ke_sys PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(ke_sys PRIVATE kernel_config)