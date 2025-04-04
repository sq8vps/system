#ifndef I686_EMU_STATE_H_
#define I686_EMU_STATE_H_

#include <stdint.h>
#include "reg.h"
#include "ke/core/mutex.h"

struct KeTaskControlBlock;

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
 * @brief Emulator state
 */
struct I686EmuState
{
    struct I686Registers registers; /**< Register state */
    uint8_t *code; /**< Mapped real mode code pointer */
    uint32_t *ivt; /**< Real mode IVT pointer within \a code */
    struct KeTaskControlBlock *owner; /**< Emulator owner */
    KeMutex mutex; /**< Emulator mutex */
};


#endif