add_subdirectory(ccs)
add_subdirectory(ces)
add_subdirectory(lib)
add_library(libc_iconv STATIC
    $<TARGET_OBJECTS:libc_iconv_ccs>
    $<TARGET_OBJECTS:libc_iconv_ces>
    $<TARGET_OBJECTS:libc_iconv_lib>
)
target_include_directories(libc_iconv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_iconv PUBLIC
    libc_iconv_ccs
    libc_iconv_ces
    libc_iconv_lib
)