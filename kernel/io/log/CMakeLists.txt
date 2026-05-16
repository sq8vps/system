add_library(io_log OBJECT
    syslog.c
    )

target_include_directories(io_log PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(io_log PRIVATE kernel_config)