add_subdirectory(${CMAKE_SYSTEM_PROCESSOR})

add_library(nabla STATIC
        wrappers.c
        $<TARGET_OBJECTS:nabla_${CMAKE_SYSTEM_PROCESSOR}>
    )
target_include_directories(nabla PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/../api/nabla)
target_link_libraries(nabla PRIVATE
    nabla_${CMAKE_SYSTEM_PROCESSOR} $<TARGET_OBJECTS:nabla_${CMAKE_SYSTEM_PROCESSOR}>
    ${CONFIG_LIBRARIES}
)

# add_custom_command(TARGET kernel POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> ${OS_IMAGE_DIR}/$<TARGET_FILE_NAME:kernel>
# )