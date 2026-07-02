add_library(libc_signal OBJECT
    psignal.c
    raise.c
    sig2str.c
    signal.c
)

target_include_directories(libc_signal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_signal PRIVATE libc_includes)