cmake_minimum_required(VERSION 3.20)
project(nabla_kernel)

add_subdirectory(ddk)
add_subdirectory(ex)
add_subdirectory(hal)
add_subdirectory(io)
add_subdirectory(it)
add_subdirectory(ke)
add_subdirectory(mm)
add_subdirectory(ob)
add_subdirectory(platform)
add_subdirectory(rtl)
add_subdirectory(templates)

# Architecture-independent kernel part
add_library(kernel_common STATIC
    main.c
    multiboot.c
)
target_include_directories(kernel_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kernel_common PUBLIC
    ddk
    ex
    io
    it
    ke
    mm
    ob
    platform
    rtl
    templates
)

# Architecture-specific complete targets

# i686
add_executable(i686 EXCLUDE_FROM_ALL)
target_link_libraries(i686 PRIVATE
    kernel_common
    hal_i686
)