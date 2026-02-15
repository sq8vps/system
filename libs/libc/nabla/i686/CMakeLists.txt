add_library(libc_nabla_i686 OBJECT
    utils.asm
)
target_link_libraries(libc_nabla_i686 PRIVATE libc_includes)