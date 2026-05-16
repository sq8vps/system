add_library(libc_ctype OBJECT
    ctype.c
)

target_include_directories(libc_ctype PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(libc_ctype PRIVATE libc_includes)