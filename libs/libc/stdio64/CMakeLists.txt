add_library(libc_stdio64 OBJECT
    fdopen64.c
    fgetpos64.c
    fopen64.c
    freopen64.c
    fseeko64.c
    fsetpos64.c
    ftello64.c
    stdio64.c
    tmpfile64.c
)

target_include_directories(libc_stdio64 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_stdio64 PRIVATE 
    libc_includes
    libc_stdio
)