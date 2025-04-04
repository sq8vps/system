#ifndef X86_EMU_REG_H_
#define X86_EMU_REG_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

struct I686InstructionParams;

EXPORT_API

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

END_EXPORT_API

/**
 * @brief Get register value in x86 real mode emulator
 * @param *reg Register structure
 * @param *prefix Instruction parameters
 * @param r Register code
 * @param segment True if segment register, false otherwise
 * @param useAddressSize True to use address size override instead of operand size override
 * @return Register value
 */
INTERNAL uint32_t I686EmuGetRegisterValue(struct I686Registers *reg, const struct I686InstructionParams *prefix, uint8_t r, bool segment, bool useAddressSize);

/**
 * @brief Set register value in x86 real mode emulator
 * @param *reg Register structure
 * @param *prefix Instruction parameters
 * @param r Register code
 * @param val Value to set
 * @param segment True if segment register, false otherwise
 */
INTERNAL void I686EmuSetRegisterValue(struct I686Registers *reg, const struct I686InstructionParams *prefix, uint8_t r, uint32_t val, bool segment);

#endif