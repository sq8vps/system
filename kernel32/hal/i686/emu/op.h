#ifndef I686_EMU_OP_H_
#define I686_EMU_OP_H_

#include <stdbool.h>
#include <stdint.h>

struct I686Registers;
struct I686InstructionParams;

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
void I686EmuMul(struct I686Registers *regs, uint32_t a, uint32_t b, uint32_t *upper, uint32_t *lower, uint8_t bits, bool sign);

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
uint32_t I686EmuAddSub(struct I686Registers *regs, uint32_t a, uint32_t b, uint8_t bits, bool add, bool withCarry);

#define I686EmuAdd(regs, a, b, bits, withCarry) I686EmuAddSub((regs), (a), (b), (bits), true, (withCarry))
#define I686EmuSub(regs, a, b, bits, withCarry) I686EmuAddSub((regs), (a), (b), (bits), false, (withCarry))

#define EMU_AND 0
#define EMU_OR 1
#define EMU_XOR 2

/**
 * @brief Perform a logic operation in x86 real mode emulator
 * @param type Operation type: #EMU_AND, #EMU_OR or #EMU_XOR
 * @param *regs Register structure
 * @param a 1st number
 * @param b 2nd number
 * @param bits Number of bits (8, 16 or 32)
 * @return Operation result
 */
uint32_t I686EmuBitwise(uint8_t type, struct I686Registers *regs, uint32_t a, uint32_t b, uint8_t bits);

#define I686EmuAnd(regs, a, b, bits) I686EmuBitwise(EMU_AND, (regs), (a), (b), (bits))
#define I686EmuOr(regs, a, b, bits) I686EmuBitwise(EMU_OR, (regs), (a), (b), (bits))
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
uint32_t I686EmuRotate(struct I686Registers *regs, uint32_t a, uint8_t count, uint8_t bits, bool right, bool withCarry);

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
uint32_t I686EmuShift(struct I686Registers *regs, uint32_t a, uint8_t count, uint8_t bits, bool right, bool arithmetic);

/**
 * @brief Perform a condition check in x86 real mode emulator
 * @param *regs CPU registers
 * @param code Jump opcode (or jump opcode equivalent)
 * @return True if condition is met, false if not
 */
bool I686EmuCheckCondition(struct I686Registers *regs, uint8_t opcode);

/**
 * @brief Perform a jump in x86 real mode emulator
 * @param *regs CPU registers
 * @param *params Instruction parameters
 * @param code Jump opcode
 * @param extended True if this is an extended opcode
 * @param coffs Code offest provided with instruction
 * @return New EIP value if branch is taken, next instruction pointer if not
 */
uint32_t I686EmuJump(struct I686Registers *regs, const struct I686InstructionParams *params, uint8_t opcode, bool extended, uint32_t coffs);


#endif