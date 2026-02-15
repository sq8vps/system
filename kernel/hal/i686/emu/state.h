/**
 * @file state.h
 * @brief Real mode emulator state structures and definitions
 * @ingroup i686_emu
 */

#ifndef I686_EMU_STATE_H_
#define I686_EMU_STATE_H_

#include <stdint.h>
#include "ke/core/mutex.h"

struct KeTaskControlBlock;

/**
 * @addtogroup i686_emu
 * @{
 */

/**
 * @brief Real mode stack top
 * 
 * This value is used to set up stack and determine the end of the execution, that is,
 * when there is an (i)ret and the stack pointer is already at the top
 */
#define I686_EMULATOR_STACK_TOP 0x6FFF0

/**
 * @brief Real mode space size used in emulator
 */
#define I686_EMU_REAL_MODE_SPACE_SIZE 0x110000

DRIVER_API

/**
 * @brief x86 registers
 */
struct I686Registers
{
    union
    {
        int32_t eax;
        int16_t ax;
        struct
        {
            int8_t al;
            int8_t ah;
        };
    };

    union
    {
        int32_t ebx;
        int16_t bx;
        struct
        {
            int8_t bl;
            int8_t bh;
        };
    };

    union
    {
        int32_t ecx;
        int16_t cx;
        struct
        {
            int8_t cl;
            int8_t ch;
        };
    };

    union
    {
        int32_t edx;
        int16_t dx;
        struct
        {
            int8_t dl;
            int8_t dh;
        };
    };

    union
    {
        int32_t esi;
        int16_t si;
        int8_t sil;
    };

    union
    {
        int32_t edi;
        int16_t di;
        int8_t dil;
    };

    union
    {
        int32_t esp;
        int16_t sp;
        int8_t spl;
    };

    union
    {
        int32_t ebp;
        int16_t bp;
        int8_t bpl;
    };


    uint16_t cs, ds, es, ss, fs, gs;
    union
    {
        uint32_t eflags;
        uint16_t flags;
    };

    union 
    {
        uint32_t eip;
        uint16_t ip;
    };
};

/**
 * @brief x86 real mode flags
 */
enum
{
    FLAG_CF = 0x1,
    FLAG_PF = 0x4,
    FLAG_AF = 0x10,
    FLAG_ZF = 0x40,
    FLAG_SF = 0x80,
    FLAG_TF = 0x100,
    FLAG_IF = 0x200,
    FLAG_DF = 0x400,
    FLAG_OF = 0x800,
};

END_DRIVER_API

/**
 * @brief Emulator state
 */
struct I686EmuState
{
    struct I686Registers registers; /**< Register state */
    uint8_t *code; /**< Mapped real mode code pointer */
    uint32_t *ivt; /**< Real mode IVT pointer within \a code */
    struct KeTaskControlBlock *owner; /**< Emulator owner */
    bool dead; /**< True if the emulator is dead, i.e. it cannot be used anymore */
    KeMutex mutex; /**< Emulator mutex */
};

/**
 * @}
 */

#endif