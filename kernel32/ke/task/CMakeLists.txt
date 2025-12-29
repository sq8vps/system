add_library(ke_task STATIC
    task.c

    tid.cpp
    )

target_include_directories(ke_task PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})