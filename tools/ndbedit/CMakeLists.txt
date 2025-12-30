cmake_minimum_required(VERSION 3.31.6)
project(NDBEdit LANGUAGES C)
set(CMAKE_C_STANDARD 23)

add_executable(ndbedit 
    main.c
    ndb.c
)
target_include_directories(ndbedit PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})