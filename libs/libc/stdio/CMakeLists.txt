add_library(libc_stdio OBJECT
    file.c
    io.c
)

target_include_directories(libc_stdio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${NABLA_API})
target_link_libraries(libc_stdio PRIVATE libc_nabla libc_stdlib libc_string)
target_link_libraries(libc_stdio PRIVATE libc_includes)