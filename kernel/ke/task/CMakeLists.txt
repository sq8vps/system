add_library(ke_task OBJECT
    cleanup.c
    task.c

    tid.cpp
    )

target_include_directories(ke_task PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(ke_task PRIVATE kernel_config)