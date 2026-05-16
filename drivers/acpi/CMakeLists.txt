set(DRIVER acpi)
add_subdirectory(acpica)

add_library(acpi_config INTERFACE)
target_compile_definitions(acpi_config INTERFACE DRIVER_NAME="${DRIVER}")


add_executable(${DRIVER} EXCLUDE_FROM_ALL
    main.c
    device.c
    init.c
    irq.c
    logging.c
    mmio.c
    pnpnames.c
    )
target_include_directories(${DRIVER} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_CURRENT_SOURCE_DIR}/acpica/include
    ${NABLA_DRIVER_API})
target_link_libraries(${DRIVER} PRIVATE
    acpi_config
    acpica
    ${CONFIG_LIBRARIES}
)


include(${NABLA_DRIVER_HELPERS})
export_symbols()
export_driver()
export_initrd_driver()