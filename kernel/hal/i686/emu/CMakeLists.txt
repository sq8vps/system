add_library(i686_emu OBJECT
    decode.c
    emu.c
    op.c
)

target_include_directories(i686_emu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_KERNEL_TOP_DIR})
target_link_libraries(i686_emu PRIVATE kernel_config)