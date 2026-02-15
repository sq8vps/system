add_library(libc_stdlib OBJECT
    call_once.c
    math.c
    mem.c
    rand.c
    conv.c
)

target_include_directories(libc_stdlib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${NABLA_API})
target_link_libraries(libc_stdlib PRIVATE libc_nabla)
target_link_libraries(libc_stdlib PRIVATE libc_includes)