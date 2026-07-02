add_library(libc_sys OBJECT
    entropy.c
    file.c    
    helpers.c
    sbrk.c
    signal.c
    task.c
    time.c
)

target_include_directories(libc_sys PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_API})
target_link_libraries(libc_sys PRIVATE libc_includes)

add_library(crt0 OBJECT
    crt0.c
)

target_include_directories(crt0 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_API})
target_link_libraries(crt0 PRIVATE libc_includes)
