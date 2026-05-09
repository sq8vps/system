add_library(dl SHARED
        elf.c
        main.c
    )
target_include_directories(dl PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${LIBC_API} ${LIBC_API}/../nabla ${NABLA_API})
target_link_directories(dl PRIVATE ${CMAKE_SOURCE_DIR}/../libs/build/libc ${CMAKE_SOURCE_DIR}/../libs/build/libnabla)
target_link_libraries(dl PRIVATE
    c
    nabla
    ${CONFIG_LIBRARIES}
)

target_link_options(dl PRIVATE "-Wl,-shared")

add_custom_command(TARGET dl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:dl> ${OS_IMAGE_DIR}/$<TARGET_FILE_NAME:dl>
)