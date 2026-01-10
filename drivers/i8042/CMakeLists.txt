set(DRIVER i8042)
add_executable(${DRIVER} EXCLUDE_FROM_ALL
    main.c
    device.c
    kbd.c
    logging.c
    main.c
    ps2.c
    )
target_include_directories(${DRIVER} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/../api)
target_link_libraries(${DRIVER} PRIVATE
    ${CONFIG_LIBRARIES}
)
target_compile_definitions(${DRIVER} PRIVATE DRIVER_NAME="${DRIVER}")

include(${DRIVER_HELPERS})
export_symbols()
export_driver()