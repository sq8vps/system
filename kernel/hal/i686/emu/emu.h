/**
 * @file emu.h
 * @brief Real mode emulator
 * @ingroup i686_emu
 */

#ifndef I686_EMU_H_
#define I686_EMU_H_

#include "defines.h"

/**
 * @addtogroup i686_emu Real mode emulator
 * @brief Real mode emulator
 * @ingroup i686
 * 
 * This module provides a x86 real mode emulator. The emulator can be used, e.g, by the graphic drivers
 * to execute video BIOS interrupts. Please note, that this emulator allows only for real-mode (<1MiB) memory
 * access and does not implement switching to protected mode.
 * @{
 */


/**
 * @brief Initialize real mode emulator
 * @return Status code
 */
INTERNAL STATUS I686InitializeEmulator(void);

EXPORT_API

/**
 * @brief Obtain linear address from segment:offset pair
 * @param seg Segment
 * @param offset Offset
 * @return Linear address
 */
#define EMU_FAR_POINTER_TO_LINEAR(seg, offset) ((uint32_t)(seg) * (uint32_t)16 + (uint32_t)(offset))

/**
 * @brief Segment used for passing data to emulator
 */
#define EMU_DATA_SEGMENT 0x7000

/**
 * @brief Offset used for passing data to emulator
 */
#define EMU_DATA_OFFSET 0x0000

struct I686Registers;

/**
 * @brief Real mode emulator status codes
 */
enum I686EmulatorState
{
    EMU_OK = 0, /**< Emulation finished successfully */
    EMU_CONTINUE = 1, /**< Emulation in progress, continue to next instruction. This status is used only internally */
    EMU_UNDEFINED_OPCODE = 2, /**< Undefined opcode encountered, emulation aborted */
    EMU_MEMORY_VIOLATION = 3, /**< Memory violation occurred, emulation aborted */
    EMU_UNAVAILABLE = 4, /**< Emulator is unavailable, probably used by another task */
};

/**
 * @brief Acquire real mode emulator
 * @param timeout Timeout in ns
 * @return Status code
 */
STATUS I686AcquireEmulator(uint64_t timeout);

/**
 * @brief Acquire real mode emulator immediately in case of a kernel panic
 * @return Status code
 * @warning Prioriy level must be HAL_PRIORITY_LEVEL_HIGHEST
 */
STATUS I686AcquireEmulatorOnPanic(void);

/**
 * @brief Release real mode emulator
 */
void I686ReleaseEmulator(void);

/**
 * @brief Emulate a real mode interrupt
 * @param vector Requested interrupt number
 * @param *regs Input/output register values
 * @param *data Data to be passed to the real mode space. Always loaded at segment ::EMU_DATA_SEGMENT, offset ::EMU_DATA_OFFSET
 * @param size Size of the passed data. If not data is to be passed, then size should be zero
 * @return Emulation result (see: ::I686EmulatorState)
 */
enum I686EmulatorState I686EmulatorDoInterrupt(uint8_t vector, struct I686Registers *regs, void *data, uint16_t size);

/**
 * @brief Read real mode memory
 * @param address Memory linear address
 * @param size Size of the data
 * @param *buffer Output buffer
 * @return Emulation result (see: ::I686EmulatorState)
 */
enum I686EmulatorState I686EmulatorReadMemory(uint32_t address, uint32_t size, void *buffer);

END_EXPORT_API

/**
 * @}
 */

#endif