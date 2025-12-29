add_subdirectory(core)
add_subdirectory(sched)
add_subdirectory(sys)
add_subdirectory(task)

add_library(ke STATIC

    )

target_include_directories(ke PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(ke PUBLIC
    ke_core
    ke_sched
    ke_sys
    ke_task
)