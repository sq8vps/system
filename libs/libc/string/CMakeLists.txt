add_library(libc_string OBJECT
    cmp.c
    concat.c
    copy.c
    misc.c
    search.c
)

target_include_directories(libc_string PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${NABLA_API})
target_link_libraries(libc_string PRIVATE libc_includes)