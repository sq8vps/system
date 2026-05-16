add_library(i686_it OBJECT
    it.c
    exceptions.c
)

target_include_directories(i686_it PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(i686_it PRIVATE kernel_config)