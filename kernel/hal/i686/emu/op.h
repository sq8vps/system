/**
 * @file op.h
 * @brief Real mode emulator operation helpers
 * @ingroup i686_emu
 */

#ifndef I686_EMU_OP_H_
#define I686_EMU_OP_H_

#include "defines.h"
#include <stdbool.h>
#include <stdint.h>

struct I686Registers;

/**
 * @addtogroup i686_emu_internal Real mode emulator internals
 * @brief Real mode emulator internals
 * @kinternal
 * @ingroup i686_emu
 * @{
 */

/**
 * @brief Instruction parameters
 */
enum
{
    REG_MOD_RM = 0x1, //reg and modr/m (/r extension)
    MOD_RM_ONLY = 0x2, //modr/m field only, reg field extends the opcode (/digit extension)
    REG_IN_OPCODE = 0x4, //no modr/m byte, register specified in lower 3 bits of the opcode (+rb, +rw, +rd extension)
    IMM_BYTE = 0x8, //immediate byte - code offset byte (ib/cb extension)
    IMM_WORD_DWORD = 0x10, //immediate (d)word
    BYTE_OPERAND = 0x20, //instruction uses 1-byte operand
    SEG_REG = 0x40, //segment register in REG field
    SEG_RM = 0x80, //segment register in RM field
    MOFFS_BYTE = 0x100, //memory offset with 1-byte operand
    MOFFS_WORD_DWORD = 0x200, //memory offset with (d)word operand
    NO_DEREFERENCE = 0x400, //do not dereference memory operand, but provide the memory address (e.g. load effective address)
    SEGMENT_CS = 0x800, //CS is the default segment register
    SEGMENT_SS = 0x1000, //SS is the default segment register
    REPE_REPZ = 0x2000, //use REPE/REPZ when REP prefix is specified
    COFFS_BYTE = 0x4000, //1-byte code offset
    COFFS_WORD = 0x8000, //2-byte code offset
    COFFS_DWORD = 0x10000, //4-byte code offset
    COFFS_PWORD = 0x20000, //6-byte code offset
    COFFS_WORD_DWORD = 0x40000, //2 or 4 byte code offset
    COFFS_DWORD_PWORD = 0x80000, //4 or 6 byte code offset
    NO_SEGMENT = 0x100000, //do not use any segment register
};

/**
 * @brief Instruction parameters, prefixes and overrides
 */
struct I686InstructionParams
{
    uint32_t rep : 1; /**< Use REP or REPE/REPZ */
    uint32_t repne : 1; /**< Use REPNE or REPNZ */
    uint32_t noModRmWrite : 1; /**< Do not write to ModR/M-encoded operand */
    struct
    {
        uint32_t cs : 1; /**< Use CS segment */
        uint32_t ss : 1; /**< Use SS segment */
        uint32_t ds : 1; /**< Use DS segment */
        uint32_t es : 1; /**< Use ES segment */
        uint32_t fs : 1; /**< Use FS segment */
        uint32_t gs : 1; /**< Use GS segment */
        uint32_t operand : 1; /**< Operand size override: 16 bits -> 32 bits */
        uint32_t address : 1; /**< Address size override: 16 bits -> 32 bits */
    } override;
    uint32_t params; /**< Instruction parameters */
};

/**
 * @brief Multiply two values in x86 real mode emulator
 * @param *regs Register structure
 * @param a 1st number
 * @param b 2nd number
 * @param *upper Upper part of the result
 * @param *lower Lower part of the result
 * @param bits Number of bits (8, 16 or 32)
 * @param sign True if signed, false if unsigned
 */
INTERNAL void I686EmuMul(struct I686Registers *regs, uint32_t a, uint32_t b, uint32_t *upper, uint32_t *lower, uint8_t bits, bool sign);

/**
 * @brief Add/subtract two values in x86 real mode emulator
 * @param *regs Register structure
 * @param a 1st number
 * @param b 2nd number
 * @param bits Number of bits (8, 16 or 32)
 * @param add True if adding, false if subtracting
 * @param withCarry True to include carry/borrow bit
 * @return Operation result (a+b or a-b)
 */
INTERNAL uint32_t I686EmuAddSub(struct I686Registers *regs, uint32_t a, uint32_t b, uint8_t bits, bool add, bool withCarry);

/**
 * @brief Add two numbers
 * @param regs Pointer to the register structure
 * @param a Addend A
 * @param b Addend B
 * @param bits Operation bit width (8, 16, or 32)
 * @param withCarry True to include carry
 * @return A + B (+ carry if \a withCarry is true)
 */
#define I686EmuAdd(regs, a, b, bits, withCarry) I686EmuAddSub((regs), (a), (b), (bits), true, (withCarry))

/**
 * @brief Subtract two numbers
 * @param regs Pointer to the register structure
 * @param a Subtrahend
 * @param b Minuend
 * @param bits Operation bit width (8, 16, or 32)
 * @param withCarry True to include carry
 * @return A - B (- carry if \a withCarry is true)
 */
#define I686EmuSub(regs, a, b, bits, withCarry) I686EmuAddSub((regs), (a), (b), (bits), false, (withCarry))

#define EMU_AND 0 /**< Bitwise AND for \a type in I686EmuBitwise */
#define EMU_OR 1 /**< Bitwise OR for \a type in I686EmuBitwise */
#define EMU_XOR 2 /**< Bitwise XOR for \a type in I686EmuBitwise */

/**
 * @brief Perform a logic operation in x86 real mode emulator
 * @param type Operation type: #EMU_AND, #EMU_OR or #EMU_XOR
 * @param *regs Register structure
 * @param a 1st number
 * @param b 2nd number
 * @param bits Number of bits (8, 16 or 32)
 * @return Operation result
 */
INTERNAL uint32_t I686EmuBitwise(uint8_t type, struct I686Registers *regs, uint32_t a, uint32_t b, uint8_t bits);

/**
 * @brief Perform a bitwise AND in x86 real mode emulator
 * @param regs Register structure
 * @param a 1st operand
 * @param b 2nd operand
 * @param bits Number of bits (8, 16 or 32)
 * @return A and B mod \a bits
 */
#define I686EmuAnd(regs, a, b, bits) I686EmuBitwise(EMU_AND, (regs), (a), (b), (bits))

/**
 * @brief Perform a bitwise OR in x86 real mode emulator
 * @param regs Register structure
 * @param a 1st operand
 * @param b 2nd operand
 * @param bits Number of bits (8, 16 or 32)
 * @return A or B mod \a bits
 */
#define I686EmuOr(regs, a, b, bits) I686EmuBitwise(EMU_OR, (regs), (a), (b), (bits))

/**
 * @brief Perform a bitwise XOR in x86 real mode emulator
 * @param regs Register structure
 * @param a 1st operand
 * @param b 2nd operand
 * @param bits Number of bits (8, 16 or 32)
 * @return A xor B mod \a bits
 */
#define I686EmuXor(regs, a, b, bits) I686EmuBitwise(EMU_XOR, (regs), (a), (b), (bits))

/**
 * @brief Perform a bit rotation in x86 real mode emulator
 * @param *regs Register structure
 * @param a Number to rotate
 * @param count Number of rotations
 * @param bits Number of bits (8, 16 or 32)
 * @param right True if right rotation, false if left rotation
 * @param withCarry True to include carry
 * @return Operation result
 */
INTERNAL uint32_t I686EmuRotate(struct I686Registers *regs, uint32_t a, uint8_t count, uint8_t bits, bool right, bool withCarry);

/**
 * @brief Perform a bit shift in x86 real mode emulator
 * @param *regs Register structure
 * @param a Number to shift
 * @param count Number of shifts
 * @param bits Number of bits (8, 16 or 32)
 * @param right True if right shift, false if left shift
 * @param arithmetic True if arithmetic shift
 * @return Operation result
 */
INTERNAL uint32_t I686EmuShift(struct I686Registers *regs, uint32_t a, uint8_t count, uint8_t bits, bool right, bool arithmetic);

/**
 * @brief Perform a condition check in x86 real mode emulator
 * @param *regs CPU registers
 * @param code Jump opcode (or jump opcode equivalent)
 * @return True if condition is met, false if not
 */
INTERNAL bool I686EmuCheckCondition(struct I686Registers *regs, uint8_t opcode);

/**
 * @brief Perform a jump in x86 real mode emulator
 * @param *regs CPU registers
 * @param *params Instruction parameters
 * @param code Jump opcode
 * @param extended True if this is an extended opcode
 * @param coffs Code offest provided with instruction
 * @return New EIP value if branch is taken, next instruction pointer if not
 */
INTERNAL uint32_t I686EmuJump(struct I686Registers *regs, const struct I686InstructionParams *params, uint8_t opcode, bool extended, uint32_t coffs);

/**
 * @brief Get register value in x86 real mode emulator
 * @param *reg Register structure
 * @param *prefix Instruction parameters
 * @param r Register code
 * @param segment True if segment register, false otherwise
 * @param useAddressSize True to use address size override instead of operand size override
 * @kinternal
 * @return Register value
 */
INTERNAL uint32_t I686EmuGetRegisterValue(struct I686Registers *reg, const struct I686InstructionParams *prefix, uint8_t r, bool segment, bool useAddressSize);

/**
 * @brief Set register value in x86 real mode emulator
 * @param *reg Register structure
 * @param *prefix Instruction parameters
 * @param r Register code
 * @param val Value to set
 * @kinternal
 * @param segment True if segment register, false otherwise
 */
INTERNAL void I686EmuSetRegisterValue(struct I686Registers *reg, const struct I686InstructionParams *prefix, uint8_t r, uint32_t val, bool segment);


/**
 * @}
 */

#endif