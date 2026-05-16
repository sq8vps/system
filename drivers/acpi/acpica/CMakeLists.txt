add_subdirectory(dispatcher)
add_subdirectory(events)
add_subdirectory(executer)
add_subdirectory(hardware)
add_subdirectory(namespace)
add_subdirectory(parser)
add_subdirectory(resources)
add_subdirectory(tables)
add_subdirectory(utilities)

add_library(acpica STATIC
    acpios.c
    )
target_include_directories(acpica PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${NABLA_DRIVER_API}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
target_link_libraries(acpica PUBLIC
    acpi_config
    dispatcher $<TARGET_OBJECTS:dispatcher>
    events $<TARGET_OBJECTS:events>
    executer $<TARGET_OBJECTS:executer>
    hardware $<TARGET_OBJECTS:hardware>
    namespace $<TARGET_OBJECTS:namespace>
    parser $<TARGET_OBJECTS:parser>
    resources $<TARGET_OBJECTS:resources>
    tables $<TARGET_OBJECTS:tables>
    utilities $<TARGET_OBJECTS:utilities>
)