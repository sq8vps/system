add_compile_definitions(_HAVE_LONG_DOUBLE=1 HAVE_RENAME=1 HAVE_MMAP=0 _LIBC=1 __NEWLIB__=1 _GNU_SOURCE=1)

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

set(LIBC_SOURCES
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

set(LIBC_LIBS
    libc_includes
    libc_iconv
    libc_machine
    nabla
    ${CONFIG_LIBRARIES}
)

set(LIBC_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${NABLA_API}
    ${NABLA_LIB_BIN_DIR}
)

add_library(c SHARED ${LIBC_SOURCES})
target_include_directories(c PRIVATE ${LIBC_INCLUDES})
target_link_libraries(c PRIVATE ${LIBC_LIBS})
target_link_options(c PRIVATE ${NABLA_SO_LINKER_FLAGS} "-Wl,-soname,$<TARGET_FILE_NAME:c>")

add_library(c-static STATIC ${LIBC_SOURCES})
target_include_directories(c PRIVATE ${LIBC_INCLUDES})
target_link_libraries(c PRIVATE ${LIBC_LIBS})

string(CONCAT NABLA_CRT0_NAME "crt0" ${CMAKE_C_OUTPUT_EXTENSION})

add_custom_command(TARGET c POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_OBJECTS:crt0> $<TARGET_FILE_DIR:c>/${NABLA_CRT0_NAME}
)

add_custom_command(TARGET c POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:c> ${NABLA_IMAGE_DIR}/lib/$<TARGET_FILE_NAME:c>
)