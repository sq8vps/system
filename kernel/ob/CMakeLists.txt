add_library(ob OBJECT
    ob.c
    )

target_include_directories(ob PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(ob PRIVATE kernel_config)