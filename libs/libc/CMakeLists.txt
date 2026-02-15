add_subdirectory(ctype)
add_subdirectory(includes)
add_subdirectory(nabla)
add_subdirectory(stdio)
add_subdirectory(stdlib)
add_subdirectory(string)

add_library(c STATIC
        $<TARGET_OBJECTS:libc_ctype>
        $<TARGET_OBJECTS:libc_nabla>
        $<TARGET_OBJECTS:libc_nabla_i686>
        $<TARGET_OBJECTS:libc_stdio>
        $<TARGET_OBJECTS:libc_stdlib>
        $<TARGET_OBJECTS:libc_string>
    )
target_include_directories(c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(c PRIVATE
    libc_includes
    ${CONFIG_LIBRARIES}
)

# add_custom_command(TARGET kernel POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> ${OS_IMAGE_DIR}/$<TARGET_FILE_NAME:kernel>
# )