add_library(libc_ssp OBJECT
    chk_fail.c
    gets_chk.c
    memcpy_chk.c
    memmove_chk.c
    mempcpy_chk.c
    memset_chk.c
    snprintf_chk.c
    sprintf_chk.c
    stack_protector.c
    stpcpy_chk.c
    stpncpy_chk.c
    strcat_chk.c
    strcpy_chk.c
    strncat_chk.c
    strncpy_chk.c
    vsnprintf_chk.c
    vsprintf_chk.c
)

target_include_directories(libc_ssp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_ssp PRIVATE libc_includes)