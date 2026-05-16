cmake_minimum_required(VERSION 3.31.6)
project(Nabla_tools LANGUAGES C CXX ASM_NASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)

add_subdirectory(ndbedit)

add_custom_target(tools)
add_dependencies(tools ndbedit)