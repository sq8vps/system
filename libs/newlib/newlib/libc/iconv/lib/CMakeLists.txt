add_library(libc_iconv_lib OBJECT
    aliasesbi.c
    aliasesi.c
    iconv.c
    iconvnls.c
    nullconv.c
    ucsconv.c
)

target_include_directories(libc_iconv_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_iconv_lib PRIVATE libc_includes)