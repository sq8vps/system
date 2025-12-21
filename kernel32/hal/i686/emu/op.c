#include "op.h"
#include <stddef.h>
#include "reg.h"
#include "state.h"

void I686EmuMul(struct I686Registers *regs, uint32_t a, uint32_t b, uint32_t *upper, uint32_t *lower, uint8_t bits, bool sign)
{
    uint32_t signMask = (uint32_t)1 << (bits - 1);
    //mask inputs to given size
    a &= (signMask | (signMask - 1));
    b &= (signMask | (signMask - 1));

    bool flags = false;
    if(8 == bits)
    {
        int16_t r;
        if(sign)
            r = (int8_t)a * (int8_t)b;
        else
            r = (uint8_t)a * (uint8_t)b;
        if((int16_t)((int8_t)r) != r)
            flags = true;
        *lower = (uint32_t)r & 0xFF;
        if(NULL != upper)
            *upper = (uint32_t)r >> 8;
    }
    else if(16 == bits)
    {
        int32_t r;
        if(sign)
            r = (int16_t)a * (int16_t)b;
        else
            r = (uint16_t)a * (uint16_t)b;
        if((int32_t)((int16_t)r) != r)
            flags = true;
        *lower = (uint32_t)r & 0xFFFF;
        if(NULL != upper)
            *upper = (uint32_t)r >> 16;
    }
    else if(32 == bits)
    {
        int64_t r;
        if(sign)
            r = (int32_t)a * (int32_t)b;
        else
            r = (uint32_t)a * (uint32_t)b;
        if((int64_t)((int32_t)r) != r)
            flags = true;
        *lower = (uint64_t)r & 0xFFFFFFFF;
        if(NULL != upper)
            *upper = (uint64_t)r >> 32;
    }
    else
        return;

    if(flags)
        regs->flags |= FLAG_OF | FLAG_CF;
    else
        regs->flags &= ~(FLAG_OF | FLAG_CF);
}

uint32_t I686EmuAddSub(struct I686Registers *regs, uint32_t a, uint32_t b, uint8_t bits, bool add, bool withCarry)
{
    uint32_t signMask = (uint32_t)1 << (bits - 1);
    uint32_t mask = (signMask | (signMask - 1));
    uint32_t carry = (withCarry && (regs->flags & FLAG_CF)) ? 1 : 0; //carry bit to be included in calculations
    
    uint32_t s = 0; //result

    //mask inputs to given size
    a &= mask;
    b &= mask;

    if(add)
    {
        uint64_t r = (uint64_t)a + (uint64_t)b + (uint64_t)carry;

        s = (uint32_t)r;

        //carry flag
        if(r & ((uint32_t)1 << bits))
            regs->flags |= FLAG_CF;
        else
            regs->flags &= ~FLAG_CF;

        //auxiliary/half-carry flag is set when the addition of the lower 4-bit nibbles generated a carry bit
        if(((a & 0xF) + (b & 0xF)) & (1 << 4))
            regs->flags |= FLAG_AF;
        else
            regs->flags &= ~FLAG_AF;
    }
    else
    {
        s = a - (b + carry);

        //carry flag
        if((b + carry) > a)
            regs->flags |= FLAG_CF;
        else
            regs->flags &= ~FLAG_CF;
        
        if(((b + carry) & 0xF) > (a & 0xF))
            regs->flags |= FLAG_AF;
        else
            regs->flags &= ~FLAG_AF;
    }

    s &= mask;
    
    if(((a & signMask) ^ (b & signMask)) & ((a & signMask) ^ (s & signMask)))
        regs->flags |= FLAG_OF;
    else
        regs->flags &= ~FLAG_OF;

    //parity flag set when the number of bits set is even
    if(0 == __builtin_parity(s))
        regs->flags &= ~FLAG_PF;
    else
        regs->flags |= FLAG_PF;

    //zero flag
    if(0 == s)
        regs->flags |= FLAG_ZF;
    else
        regs->flags &= ~FLAG_ZF;

    //sign flag
    if(s & signMask)
        regs->flags |= FLAG_SF;
    else
        regs->flags &= ~FLAG_SF;

    return s;
}

#define I686EmuAdd(regs, a, b, bits, withCarry) I686EmuAddSub((regs), (a), (b), (bits), true, (withCarry))
#define I686EmuSub(regs, a, b, bits, withCarry) I686EmuAddSub((regs), (a), (b), (bits), false, (withCarry))

#define EMU_AND 0
#define EMU_OR 1
#define EMU_XOR 2

uint32_t I686EmuBitwise(uint8_t type, struct I686Registers *regs, uint32_t a, uint32_t b, uint8_t bits)
{
    uint32_t mask = ((uint64_t)1 << bits) - (uint64_t)1;
    uint32_t r = 0;
    switch(type)
    {
        case EMU_AND:
            r = a & b;
            break;
        case EMU_OR:
            r = a | b;
            break;
        case EMU_XOR:
            r = a ^ b;
            break;
    }

    r &= mask;

    regs->flags &= ~(FLAG_CF | FLAG_OF);

    //parity flag set when the number of bits set is even
    if(0 == __builtin_parity(r))
        regs->flags &= ~FLAG_PF;
    else
        regs->flags |= FLAG_PF;

    //zero flag
    if(0 == r)
        regs->flags |= FLAG_ZF;
    else
        regs->flags &= ~FLAG_ZF;

    //sign flag
    if(r & ((uint32_t)1 << (bits - 1)))
        regs->flags |= FLAG_SF;
    else
        regs->flags &= ~FLAG_SF;

    return r;
}

#define I686EmuAnd(regs, a, b, bits) I686EmuBitwise(EMU_AND, (regs), (a), (b), (bits))
#define I686EmuOr(regs, a, b, bits) I686EmuBitwise(EMU_OR, (regs), (a), (b), (bits))
#define I686EmuXor(regs, a, b, bits) I686EmuBitwise(EMU_XOR, (regs), (a), (b), (bits))

uint32_t I686EmuRotate(struct I686Registers *regs, uint32_t a, uint8_t count, uint8_t bits, bool right, bool withCarry)
{
    if(right)
    {
        if(withCarry)
        {
            if(1 == count)
            {
                regs->flags &= ~FLAG_OF;
                if((!!(a & (1 << (bits - 1))) ^ !!(regs->flags & FLAG_CF)))
                    regs->flags |= FLAG_OF;
            }
            for(uint8_t i = 0; i < count; i++)
            {
                uint8_t cf = a & 1;
                a >>= 1;
                a |= ((!!(regs->flags & FLAG_CF)) << (bits - 1));
                if(cf)
                    regs->flags |= FLAG_CF;
                else
                    regs->flags &= ~FLAG_CF;
            }
        }
        else
        {
            for(uint8_t i = 0; i < count; i++)
            {
                uint8_t cf = a & 1;
                a >>= 1;
                a |= (cf << (bits - 1));
            }
            if(0 != count)
            {
                regs->flags &= ~(FLAG_CF | FLAG_OF);
                if(a & (1 << (bits - 1)))
                    regs->flags |= FLAG_CF;
                if((1 == count) && (!!(a & (1 << (bits - 1))) ^ !!(regs->flags & FLAG_CF)))
                    regs->flags |= FLAG_OF;
            }
        }
    }
    else
    {
        if(withCarry)
        {
            for(uint8_t i = 0; i < count; i++)
            {
                uint8_t cf = !!(a & (1 << (bits - 1)));
                a <<= 1;
                a |= !!(regs->flags & FLAG_CF);
                if(cf)
                    regs->flags |= FLAG_CF;
                else
                    regs->flags &= ~FLAG_CF;
            }
            if(1 == count)
            {
                regs->flags &= ~FLAG_OF;
                if((!!(a & (1 << (bits - 1))) ^ !!(regs->flags & FLAG_CF)))
                    regs->flags |= FLAG_OF;
            }
        }
        else
        {
            for(uint8_t i = 0; i < count; i++)
            {
                uint8_t cf = !!(a & (1 << (bits - 1)));
                a <<= 1;
                a |= cf;
            }
            if(0 != count)
            {
                regs->flags &= ~(FLAG_CF | FLAG_OF);
                if(a & 1)
                    regs->flags |= FLAG_CF;
                if((1 == count) && (!!(a & (1 << (bits - 1))) ^ !!(regs->flags & FLAG_CF)))
                    regs->flags |= FLAG_OF;
            }
        }
    }

    return a;
}

uint32_t I686EmuShift(struct I686Registers *regs, uint32_t a, uint8_t count, uint8_t bits, bool right, bool arithmetic)
{
    uint32_t sign = a & (1 << (bits - 1));
    for(uint8_t i = 0; i < count; i++)
    {
        if(!right)
        {
            if(a & (1 << (bits - 1)))
                regs->flags |= FLAG_CF;
            else
                regs->flags &= ~FLAG_CF;
            
            a <<= 1;
        }
        else
        {
            if(a & 1)
                regs->flags |= FLAG_CF;
            else
                regs->flags &= ~FLAG_CF;
            
            a >>= 1;
            if(arithmetic)
                a |= sign;
        }
    }

    if(0 != count)
    {
        uint8_t msb = !!(a & (1 << (bits - 1)));
        regs->flags &= ~(FLAG_SF | FLAG_ZF | FLAG_PF | FLAG_OF);
        if(msb)
            regs->flags |= FLAG_SF;
        if(0 == a)
            regs->flags |= FLAG_ZF;
        if(!__builtin_parity(a))
            regs->flags |= FLAG_PF;
        
        if(1 == count)
        {
            if(!right)
            {
                if(msb ^ !!(regs->flags & FLAG_CF))
                    regs->flags |= FLAG_OF;
            }
            else if(!arithmetic && msb) //shr and msb is set?
                regs->flags |= FLAG_OF;

        }
    }
    return a;
}

bool I686EmuCheckCondition(struct I686Registers *regs, uint8_t opcode)
{
    uint16_t flags = regs->flags;
    bool success = false;

    switch(opcode)
    {
        case 0x77:
            success = !(flags & (FLAG_CF | FLAG_ZF));
            break;
        case 0x73:
            success = !(flags & FLAG_CF);
            break;
        case 0x72:
            success = !!(flags & FLAG_CF);
            break;
        case 0x76:
            success = !!(flags & (FLAG_CF | FLAG_ZF));
            break;
        case 0x74:
            success = !!(flags & FLAG_ZF);
            break;
        case 0x7F:
            success = !(flags & FLAG_ZF) && (!!(flags & FLAG_SF) == !!(flags & FLAG_OF));
            break;
        case 0x7D:
            success = (!!(flags & FLAG_SF) == !!(flags & FLAG_OF));
            break;
        case 0x7C:
            success = (!!(flags & FLAG_SF) != !!(flags & FLAG_OF));
            break;
        case 0x7E:
            success = !!(flags & FLAG_ZF) || (!!(flags & FLAG_SF) != !!(flags & FLAG_OF));
            break;
        case 0x75:
            success = !(flags & FLAG_ZF);
            break;
        case 0x71:
            success = !(flags & FLAG_OF);
            break;
        case 0x7B:
            success = !(flags & FLAG_PF);
            break;
        case 0x79:
            success = !(flags & FLAG_SF);
            break;
        case 0x70:
            success = !!(flags & FLAG_OF);
            break;
        case 0x7A:
            success = !!(flags & FLAG_PF);
            break;
        case 0x78:
            success = !!(flags & FLAG_SF);
            break;
    }

    return success;
}


uint32_t I686EmuJump(struct I686Registers *regs, const struct I686InstructionParams *params, uint8_t opcode, bool extended, uint32_t coffs)
{
    if(extended)
        opcode -= 0x10;
    
    bool taken = false;

    switch(opcode)
    {
        case 0xE2: //loop
        case 0xE1: //loope
        case 0xE0: //loopne
            if(params->override.address)
                taken = (0 != --regs->ecx);
            else
                taken = (0 != --regs->cx);
            if(0xE1 == opcode)
                taken = !!(regs->flags & FLAG_ZF) && taken;
            else if(0xE2 == opcode)
                taken = !(regs->flags & FLAG_ZF) && taken;
            break;
        default:
            taken = I686EmuCheckCondition(regs, opcode);
            break;
    }

    if(!extended)
        return regs->eip + 2 + (taken ? (int8_t)coffs : 0);
    else if(params->override.operand)
        return regs->eip + 5 + (taken ? (int32_t)coffs : 0);
    else
        return regs->eip + 3 + (taken ? (int16_t)coffs : 0);
}
