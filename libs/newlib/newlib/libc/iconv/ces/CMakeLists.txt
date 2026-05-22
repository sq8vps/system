add_library(libc_iconv_ces OBJECT
    cesbi.c
    euc.c
    table-pcs.c
    table.c
    ucs-2-internal.c
    ucs-2.c
    ucs-4-internal.c
    ucs-4.c
    us-ascii.c
    utf-8.c
    utf-16.c
)

target_include_directories(libc_iconv_ces PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_iconv_ces PRIVATE libc_includes)