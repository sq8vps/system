add_subdirectory(ctype)
add_subdirectory(includes)
add_subdirectory(nabla)
add_subdirectory(stdio)
add_subdirectory(stdlib)
add_subdirectory(string)

set(LIBC_SOURCES
    $<TARGET_OBJECTS:libc_ctype>
    $<TARGET_OBJECTS:libc_nabla>
    $<TARGET_OBJECTS:libc_nabla_i686>
    $<TARGET_OBJECTS:libc_stdio>
    $<TARGET_OBJECTS:libc_stdlib>
    $<TARGET_OBJECTS:libc_string>
)
set(LIBC_LIBS
    libc_includes
    ${CONFIG_LIBRARIES}
)
set(LIBC_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(c-static STATIC ${LIBC_SOURCES})
target_include_directories(c-static PUBLIC ${LIBC_INCLUDES})
target_link_libraries(c-static PUBLIC ${LIBC_LIBS})

add_library(c SHARED ${LIBC_SOURCES}) 
target_include_directories(c PUBLIC ${LIBC_INCLUDES})
target_link_libraries(c PUBLIC ${LIBC_LIBS} nabla)
target_link_options(c PRIVATE ${NABLA_SO_LINKER_FLAGS} "-Wl,-soname,$<TARGET_FILE_NAME:c>")

# add_custom_command(TARGET kernel POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> ${NABLA_IMAGE_DIR}/$<TARGET_FILE_NAME:kernel>
# )