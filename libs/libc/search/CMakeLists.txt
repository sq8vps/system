add_library(libc_search OBJECT
    bsd_qsort_r.c
    bsearch.c
    hash_bigkey.c
    hash_buf.c
    hash_func.c
    hash_log2.c
    hash_page.c
    hash.c
    hcreate_r.c
    hcreate.c
    ndbm.c
    qsort_r.c
    qsort.c
    tdelete.c
    tdestroy.c
    tfind.c
    tsearch.c
    twalk.c
)

target_include_directories(libc_search PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(libc_search PRIVATE libc_includes)