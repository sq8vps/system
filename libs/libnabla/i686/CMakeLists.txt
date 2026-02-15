add_library(nabla_i686 OBJECT
    syscall.asm
)
target_include_directories(nabla_i686 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})