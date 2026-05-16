add_library(multiboot STATIC
    multiboot.c
)

target_include_directories(multiboot PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(multiboot PRIVATE kernel_config)