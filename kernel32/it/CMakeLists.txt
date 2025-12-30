add_library(it OBJECT
    it.c
    )

target_include_directories(it PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(it PRIVATE kernel_config)