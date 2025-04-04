#include "reg.h"
#include "state.h"

uint32_t I686EmuGetRegisterValue(struct I686Registers *reg, const struct I686InstructionParams *prefix, uint8_t r, bool segment, bool useAddressSize)
{
    uint32_t val = 0;
    if(segment)
    {
        switch(r)
        {
            case 0:
                val = reg->es;
                break;
            case 1:
                val = reg->cs;
                break;
            case 2:
                val = reg->ss;
                break;
            case 3:
                val = reg->ds;
                break;
            case 4:
                val = reg->fs;
                break;
            case 5:
                val = reg->gs;
                break;
            default:
                break;
        }
    }
    else
    {
        if((prefix->params & BYTE_OPERAND) && !useAddressSize)
        {
            switch(r) 
            {
                case 0:
                    val = reg->al;
                    break;
                case 1:
                    val = reg->cl;
                    break;
                case 2: 
                    val = reg->dl;
                    break;
                case 3: 
                    val = reg->bl;
                    break;
                case 4: 
                    val = reg->ah;
                    break;
                case 5:
                    val = reg->ch;
                    break;
                case 6:
                    val = reg->dh;
                    break;
                case 7:
                    val = reg->bh;
                    break;
                default:
                    break;
            }
        }
        else if(useAddressSize ? prefix->override.address : prefix->override.operand)
        {
            switch(r) 
            {
                case 0:
                    val = reg->eax;
                    break;
                case 1:
                    val = reg->ecx;
                    break;
                case 2:
                    val = reg->edx;
                    break;
                case 3:
                    val = reg->ebx;
                    break;
                case 4:
                    val = reg->esp;
                    break;
                case 5:
                    val = reg->ebp;
                    break;
                case 6:
                    val = reg->esi;
                    break;
                case 7:
                    val = reg->edi;
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch(r) 
            {
                case 0:
                    val = reg->ax;
                    break;
                case 1:
                    val = reg->cx;
                    break;
                case 2:
                    val = reg->dx;
                    break;
                case 3:
                    val = reg->bx;
                    break;
                case 4:
                    val = reg->sp;
                    break;
                case 5:
                    val = reg->bp;
                    break;
                case 6:
                    val = reg->si;
                    break;
                case 7:
                    val = reg->di;
                    break;
                default:
                    break;
            }
        }
    }

    return val;
}

void I686EmuSetRegisterValue(struct I686Registers *reg, const struct I686InstructionParams *prefix, uint8_t r, uint32_t val, bool segment)
{
    if(segment)
    {
        switch(r)
        {
            case 0:
                reg->es = val;
                break;
            case 1:
                reg->cs = val;
                break;
            case 2:
                reg->ss = val;
                break;
            case 3:
                reg->ds = val;
                break;
            case 4:
                reg->fs = val;
                break;
            case 5:
                reg->gs = val;
                break;
            default:
                break;
        }
    }
    else
    {
        if(prefix->params & BYTE_OPERAND)
        {
            switch(r) 
            {
                case 0:
                    reg->al = val;
                    break;
                case 1:
                    reg->cl = val;
                    break;
                case 2: 
                    reg->dl = val;
                    break;
                case 3: 
                    reg->bl = val;
                    break;
                case 4: 
                    reg->ah = val;
                    break;
                case 5:
                    reg->ch = val;
                    break;
                case 6:
                    reg->dh = val;
                    break;
                case 7:
                    reg->bh = val;
                    break;
                default:
                    break;
            }
        }
        else if(prefix->override.operand)
        {
            switch(r) 
            {
                case 0:
                    reg->eax = val;
                    break;
                case 1:
                    reg->ecx = val;
                    break;
                case 2:
                    reg->edx = val;
                    break;
                case 3:
                    reg->ebx = val;
                    break;
                case 4:
                    reg->esp = val;
                    break;
                case 5:
                    reg->ebp = val;
                    break;
                case 6:
                    reg->esi = val;
                    break;
                case 7:
                    reg->edi = val;
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch(r) 
            {
                case 0:
                    reg->ax = val;
                    break;
                case 1:
                    reg->cx = val;
                    break;
                case 2:
                    reg->dx = val;
                    break;
                case 3:
                    reg->bx = val;
                    break;
                case 4:
                    reg->sp = val;
                    break;
                case 5:
                    reg->bp = val;
                    break;
                case 6:
                    reg->si = val;
                    break;
                case 7:
                    reg->di = val;
                    break;
                default:
                    break;
            }
        }
    }
}