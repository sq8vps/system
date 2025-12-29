add_library(i686_it STATIC
    it.c
    exceptions.c
)

target_include_directories(i686_it PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)