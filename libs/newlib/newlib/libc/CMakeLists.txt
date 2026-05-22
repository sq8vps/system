add_subdirectory(argz)
add_subdirectory(ctype)
add_subdirectory(errno)
add_subdirectory(iconv)
add_subdirectory(include)
add_subdirectory(locale)
add_subdirectory(machine)
add_subdirectory(misc)
#add_subdirectory(posix)
add_subdirectory(reent)
add_subdirectory(search)
add_subdirectory(signal)
add_subdirectory(ssp)
add_subdirectory(stdbit)
add_subdirectory(stdio)
#add_subdirectory(stdio64)
add_subdirectory(stdlib)
add_subdirectory(string)
add_subdirectory(sys)
add_subdirectory(syscalls)
add_subdirectory(time)
# add_subdirectory(unix)
# add_subdirectory(xdr)

add_library(c SHARED
        $<TARGET_OBJECTS:libc_argz>
        $<TARGET_OBJECTS:libc_ctype>
        $<TARGET_OBJECTS:libc_errno>
        $<TARGET_OBJECTS:libc_locale>
        $<TARGET_OBJECTS:libc_misc>
#        $<TARGET_OBJECTS:libc_posix>
        $<TARGET_OBJECTS:libc_reent>
        $<TARGET_OBJECTS:libc_search>
        $<TARGET_OBJECTS:libc_signal>
        $<TARGET_OBJECTS:libc_ssp>
        $<TARGET_OBJECTS:libc_stdbit>
        $<TARGET_OBJECTS:libc_stdio>
#        $<TARGET_OBJECTS:libc_stdio64>
        $<TARGET_OBJECTS:libc_stdlib>
        $<TARGET_OBJECTS:libc_string>
        $<TARGET_OBJECTS:libc_sys>
        $<TARGET_OBJECTS:libc_syscalls>
        $<TARGET_OBJECTS:libc_time>
    )
target_include_directories(c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(c PRIVATE
    libc_includes
    libc_iconv
    libc_machine
    ${CONFIG_LIBRARIES}
)
target_link_options(c PRIVATE "-Wl,-shared")