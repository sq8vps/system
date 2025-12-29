add_subdirectory(emu)
add_subdirectory(interrupts)

add_library(i686_hal STATIC
    acpi.c
    cpu.c
    dcpuid.c
    fpu.c
    gdt.c
    i686.c
    ioapic.c
    ioport.c
    ipi.c
    irq.c
    lapic.c
    math.c
    memory.c
    msr.c
    pic.c
    pit.c
    root.c
    sse.c
    syscall.c
    task.c
    tsc.c
    uart.c
    video.c

    utils.cpp

    apboot.asm
    boot.asm
    llsyscall.asm
    switch.asm
)

target_include_directories(i686_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(i686_hal PUBLIC
    i686_emu
    i686_it
    hal
    )