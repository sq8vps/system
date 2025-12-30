add_library(platform INTERFACE

)

target_include_directories(platform INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(platform INTERFACE kernel_config)