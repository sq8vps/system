add_executable(init
        main.c
        tty.c
    )
target_include_directories(init PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${LIBC_API} ${LIBC_API}/../nabla ${NABLA_API})
target_link_directories(init PRIVATE ${CMAKE_SOURCE_DIR}/../libs/build/libc ${CMAKE_SOURCE_DIR}/../libs/build/libnabla)
target_link_libraries(init PRIVATE
    c
    nabla
    ${CONFIG_LIBRARIES}
)

add_custom_command(TARGET init POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:init> ${OS_IMAGE_DIR}/$<TARGET_FILE_NAME:init>
)