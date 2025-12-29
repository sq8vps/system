add_library(i686_emu STATIC
    decode.c
    emu.c
    op.c
)

target_include_directories(i686_emu PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)