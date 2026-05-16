cmake_minimum_required(VERSION 3.31.6)
project(Nabla_kernel LANGUAGES C CXX ASM_NASM)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)
set(CMAKE_EXECUTABLE_SUFFIX "")

include(../nabla.cmake)

set(NABLA_KERNEL_TOP_DIR ${CMAKE_CURRENT_SOURCE_DIR})

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
add_subdirectory(multiboot)

add_library(kernel_config INTERFACE)
if(SMP EQUAL 1)
    target_compile_definitions(kernel_config INTERFACE SMP=1)
endif()
if(CONFIG_USE_MULTIBOOT2)
    target_compile_definitions(kernel_config INTERFACE MULTIBOOT2=1)
endif()

add_executable(kernel
    main.c
    config.c
    )
target_include_directories(kernel PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(kernel PRIVATE
    platform
    templates
    ddk
    ex
    hal
    io
    it
    ke
    mm
    ob
    rtl
    "${CMAKE_SYSTEM_PROCESSOR}_hal"
    ${CONFIG_LIBRARIES}
)
target_link_libraries(kernel PRIVATE kernel_config)

add_custom_command(TARGET kernel POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> ${NABLA_IMAGE_DIR}/$<TARGET_FILE_NAME:kernel>
)

include("target-helpers.cmake")