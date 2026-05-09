cmake_minimum_required(VERSION 3.31.6)
project(Nabla_base LANGUAGES C CXX ASM_NASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

#add_link_options(${LINKER_FLAGS})

add_subdirectory(init)
add_subdirectory(dl)

add_custom_target(base)
add_dependencies(base init dl)