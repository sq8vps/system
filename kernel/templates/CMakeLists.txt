add_library(templates INTERFACE

)

target_include_directories(templates INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(templates INTERFACE kernel_config)