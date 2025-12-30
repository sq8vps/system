add_library(ke_task OBJECT
    task.c

    tid.cpp
    )

target_include_directories(ke_task PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(ke_task PRIVATE kernel_config)