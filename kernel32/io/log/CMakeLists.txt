add_library(io_log STATIC
    syslog.c
    )

target_include_directories(io_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})