add_library(libc_time OBJECT
   asctime_r.c
    ctime_r.c
    gmtime.c
    lcltime_r.c
    month_lengths.c
    time.c
    tzlock.c
    tzvars.c
    clock.c
    difftime.c
    gmtime_r.c
    strftime.c
    tzset.c
    wcsftime.c
    asctime.c
    ctime.c
    gettzinfo.c
    lcltime.c
    mktime.c
    strptime.c
    tzcalc_limits.c
    tzset_r.c
)

target_include_directories(libc_time PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_time PRIVATE libc_includes)