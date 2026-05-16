set(DRIVER tty)
add_executable(${DRIVER} EXCLUDE_FROM_ALL
    main.c
    device.c
    font.c
    keymap.c
    logging.c
    read.c
    vt.c
    write.c
    )
target_include_directories(${DRIVER} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${NABLA_DRIVER_API})
target_link_libraries(${DRIVER} PRIVATE
    ${CONFIG_LIBRARIES}
)
target_compile_definitions(${DRIVER} PRIVATE DRIVER_NAME="${DRIVER}")

include(${NABLA_DRIVER_HELPERS})
export_symbols()
export_driver()