cmake_minimum_required(VERSION 3.31.6)
project(Nabla_libs LANGUAGES C CXX ASM_NASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

add_subdirectory(libnabla)
add_subdirectory(libc)

add_custom_target(libs)
add_dependencies(libs nabla c)