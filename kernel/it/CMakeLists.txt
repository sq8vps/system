add_library(it OBJECT
    it.c
    )

target_include_directories(it PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(it PRIVATE kernel_config)