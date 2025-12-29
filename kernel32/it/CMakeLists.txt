add_library(it STATIC
    it.c
    )

target_include_directories(it PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})