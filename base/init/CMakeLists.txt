add_executable(init
        main.c
        tty.c
        ${NABLA_LIB_BIN_DIR}/libc/crt0.o
    )
target_include_directories(init PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_LIB_INCLUDE_DIR}/libnabla ${NABLA_LIB_INCLUDE_DIR}/libc/include ${NABLA_API})

target_include_directories(init PRIVATE ${NABLA_LIB_BIN_DIR}/libnabla ${NABLA_LIB_BIN_DIR}/libc)
target_link_directories(init PRIVATE ${NABLA_LIB_BIN_DIR}/libnabla ${NABLA_LIB_BIN_DIR}/libc)
target_link_libraries(init PRIVATE
    c
    nabla
    ${CONFIG_LIBRARIES}
)

target_link_options(init PRIVATE ${NABLA_EXE_LINKER_FLAGS})

add_custom_command(TARGET init POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:init> ${NABLA_IMAGE_DIR}/base/$<TARGET_FILE_NAME:init>
)