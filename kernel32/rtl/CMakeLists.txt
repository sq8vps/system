add_library(rtl STATIC
    bst.c
    crc.c
    order.c
    random.c
    ring.c
    rtl.c
    stdio.c
    trie.c
    uuid.c
)

target_include_directories(rtl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})