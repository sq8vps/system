add_library(i686_it OBJECT
    it.c
    exceptions.c
)

target_include_directories(i686_it PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(i686_it PRIVATE kernel_config)