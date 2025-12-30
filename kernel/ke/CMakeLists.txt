add_subdirectory(core)
add_subdirectory(sched)
add_subdirectory(sys)
add_subdirectory(task)

add_library(ke INTERFACE)
target_link_libraries(ke INTERFACE
    ke_core $<TARGET_OBJECTS:ke_core>
    ke_sched $<TARGET_OBJECTS:ke_sched>
    ke_sys $<TARGET_OBJECTS:ke_sys>
    ke_task $<TARGET_OBJECTS:ke_task>
)
target_link_libraries(ke INTERFACE kernel_config)