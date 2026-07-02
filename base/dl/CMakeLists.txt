add_library(dl SHARED
        elf.c
        libs.c
        main.c
        start.c
    )
target_include_directories(dl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_LIB_INCLUDE_DIR}/libnabla ${NABLA_LIB_INCLUDE_DIR}/libc/include ${NABLA_API})

target_include_directories(dl PRIVATE ${NABLA_LIB_BIN_DIR}/libnabla ${NABLA_LIB_BIN_DIR}/libc)
target_link_directories(dl PRIVATE ${NABLA_LIB_BIN_DIR}/libnabla ${NABLA_LIB_BIN_DIR}/libc)
target_link_libraries(dl PRIVATE
    c-static
    nabla-static
    ${CONFIG_LIBRARIES}
)

target_link_options(dl PRIVATE ${NABLA_SO_LINKER_FLAGS})

add_custom_command(TARGET dl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:dl> ${NABLA_IMAGE_DIR}/base/$<TARGET_FILE_NAME:dl>
)