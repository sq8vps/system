add_library(multiboot STATIC
    multiboot.c
)

target_include_directories(multiboot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(multiboot PRIVATE kernel_config)