add_subdirectory(${CMAKE_SYSTEM_PROCESSOR})

add_library(libc_nabla OBJECT
    errno.c
    init.c
    tls.c
)

target_include_directories(libc_nabla PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${NABLA_API})
target_link_libraries(libc_nabla PUBLIC 
    libc_includes
    libc_nabla_${CMAKE_SYSTEM_PROCESSOR} $<TARGET_OBJECTS:libc_nabla_${CMAKE_SYSTEM_PROCESSOR}>)