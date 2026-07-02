cmake_minimum_required(VERSION 3.31.6)
project(Nabla_libs LANGUAGES C CXX ASM ASM_NASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

include(../nabla.cmake)

add_subdirectory(libnabla)
add_subdirectory(libc)

add_custom_target(libs)
add_dependencies(libs nabla nabla-static crt0 c c-static)