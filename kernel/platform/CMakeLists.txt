add_library(platform INTERFACE

)

target_include_directories(platform INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(platform INTERFACE kernel_config)