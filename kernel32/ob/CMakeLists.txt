add_library(ob STATIC
    ob.c
    )

target_include_directories(ob PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})