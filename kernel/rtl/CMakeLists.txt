add_library(rtl OBJECT
    bst.c
    crc.c
    order.c
    random.c
    ring.c
    rtl.c
    stdio.c
    trie.c
    user.c
    uuid.c
)

target_include_directories(rtl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(rtl PRIVATE kernel_config)