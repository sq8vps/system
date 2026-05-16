add_library(templates INTERFACE

)

target_include_directories(templates INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(templates INTERFACE kernel_config)