add_subdirectory(${CMAKE_SYSTEM_PROCESSOR})

add_library(libc_machine INTERFACE)
target_include_directories(libc_machine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_machine INTERFACE
    libc_machine_${CMAKE_SYSTEM_PROCESSOR}
)