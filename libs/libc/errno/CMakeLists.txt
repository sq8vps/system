add_library(libc_errno OBJECT
    errno.c
)

target_include_directories(libc_errno PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_errno PRIVATE libc_includes)