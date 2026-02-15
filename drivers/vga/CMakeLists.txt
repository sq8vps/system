set(DRIVER vga)
add_executable(${DRIVER} EXCLUDE_FROM_ALL
    main.c
    logging.c
    vesa.c
    vga.c
    )
target_include_directories(${DRIVER} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${DRIVER_API})
target_link_libraries(${DRIVER} PRIVATE
    ${CONFIG_LIBRARIES}
)
target_compile_definitions(${DRIVER} PRIVATE DRIVER_NAME="${DRIVER}")

include(${DRIVER_HELPERS})
export_symbols()
export_driver()