add_library(libc_sys OBJECT
    helpers.c
    signal.c
)

target_include_directories(libc_sys PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_sys PRIVATE libc_includes)