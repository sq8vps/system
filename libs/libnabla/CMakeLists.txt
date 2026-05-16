add_subdirectory(${CMAKE_SYSTEM_PROCESSOR})

set(LIBNABLA_SOURCES
    wrappers.c
    $<TARGET_OBJECTS:nabla_${CMAKE_SYSTEM_PROCESSOR}> 
)
set(LIBNABLA_LIBS
    nabla_${CMAKE_SYSTEM_PROCESSOR}
    ${CONFIG_LIBRARIES}
)
set(LIBNABLA_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${NABLA_API}
)

add_library(nabla-static STATIC ${LIBNABLA_SOURCES})
target_include_directories(nabla-static PUBLIC ${LIBNABLA_INCLUDES})
target_link_libraries(nabla-static PUBLIC ${LIBNABLA_LIBS})

add_library(nabla SHARED ${LIBNABLA_SOURCES})
target_include_directories(nabla PUBLIC ${LIBNABLA_INCLUDES})
target_link_libraries(nabla PUBLIC ${LIBNABLA_LIBS})
target_link_options(nabla PRIVATE ${NABLA_SO_LINKER_FLAGS} "-Wl,-soname,$<TARGET_FILE_NAME:nabla>")
set_target_properties(nabla PROPERTIES
    VERSION 1.0
    SOVERSION 1
)

# add_custom_command(TARGET kernel POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:kernel> ${NABLA_IMAGE_DIR}/$<TARGET_FILE_NAME:kernel>
# )