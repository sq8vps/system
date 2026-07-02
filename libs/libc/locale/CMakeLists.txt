add_library(libc_locale OBJECT
    duplocale.c
    freelocale.c
    getlocalename_l.c
    lctype.c
    lmessages.c
    lmonetary.c
    lnumeric.c
    locale.c
    localeconv.c
    newlocale.c
    nl_langinfo.c
    timelocal.c
    uselocale.c
)

target_include_directories(libc_locale PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_locale PRIVATE libc_includes)