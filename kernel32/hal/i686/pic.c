#if defined(__i686__) || defined(__amd64__)

#include "pic.h"
#include "ioport.h"
#include "ke/core/mutex.h"
#include "hal/interrupt.h"

#define PIC_MASTER_CMD_PORT 0x0020
#define PIC_MASTER_DATA_PORT 0x0021
#define PIC_SLAVE_CMD_PORT 0x00A0
#define PIC_SLAVE_DATA_PORT 0x00A1

#define PIC_CMD_EOI 0x20 //end of interrupt command
#define PIC_CMD_READ_IRR 0x0A
#define PIC_CMD_READ_ISR 0x0B

#define PIC_ICW1_FLAG_IC4 0x01 //indicate that ICW4 will be present
#define PIC_ICW1_FLAG_INIT 0x10	//constant bit set to 1
#define PIC_ICW4_FLAG_8086 0x01 //8086 mode

#define IS_MASTER_INPUT(x) (((x) >= 0) && ((x) <= 7))
#define IS_SLAVE_INPUT(x) (((x) >= 8) && ((x) <= 15))
#define CHECK_INPUT(x) (IS_MASTER_INPUT(x) || IS_SLAVE_INPUT(x))

struct PicEntry
{
    KeSpinlock mutex;
    uint8_t usage;
} static Pic[2];

STATUS PicSendEoi(uint32_t input)
{
    if(!CHECK_INPUT(input))
        return IT_BAD_VECTOR;
    
    if(IS_SLAVE_INPUT(input))
    {
        PRIO prio = KeAcquireSpinlock(&(Pic[1].mutex));
        IoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_EOI);
        KeReleaseSpinlock(&(Pic[1].mutex), prio);
    }

    PRIO prio = KeAcquireSpinlock(&(Pic[0].mutex));
    IoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_EOI);
    KeReleaseSpinlock(&(Pic[0].mutex), prio);
    return OK;
}

void PicRemap(uint8_t masterIrqOffset, uint8_t slaveIrqOffset)
{
    PRIO prio = KeAcquireSpinlock(&(Pic[0].mutex));
    uint8_t mMask = IoPortReadByte(PIC_MASTER_DATA_PORT);
    IoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    IoPortWriteByte(PIC_MASTER_DATA_PORT, masterIrqOffset);
    IoPortWriteByte(PIC_MASTER_DATA_PORT, 4);
    IoPortWriteByte(PIC_MASTER_DATA_PORT, PIC_ICW4_FLAG_8086);
    IoPortWriteByte(PIC_MASTER_DATA_PORT, mMask);
    KeReleaseSpinlock(&(Pic[0].mutex), prio);

    prio = KeAcquireSpinlock(&(Pic[1].mutex));
    uint8_t sMask = IoPortReadByte(PIC_SLAVE_DATA_PORT);
    IoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    IoPortWriteByte(PIC_SLAVE_DATA_PORT, slaveIrqOffset);
    IoPortWriteByte(PIC_SLAVE_DATA_PORT, 2);
    IoPortWriteByte(PIC_SLAVE_DATA_PORT, PIC_ICW4_FLAG_8086);
    IoPortWriteByte(PIC_SLAVE_DATA_PORT, sMask);
    KeReleaseSpinlock(&(Pic[1].mutex), prio);
}

STATUS PicDisableIrq(uint32_t input)
{
    if(!CHECK_INPUT(input))
        return IT_BAD_VECTOR;

    if(IS_MASTER_INPUT(input))
    {
        PRIO prio = KeAcquireSpinlock(&(Pic[0].mutex));
        IoPortWriteByte(PIC_MASTER_DATA_PORT, IoPortReadByte(PIC_MASTER_DATA_PORT) | (1 << input));
        KeReleaseSpinlock(&(Pic[0].mutex), prio);
    }
    else
    {
        PRIO prio = KeAcquireSpinlock(&(Pic[1].mutex));
        IoPortWriteByte(PIC_MASTER_DATA_PORT, IoPortReadByte(PIC_SLAVE_DATA_PORT) | (1 << (input - 8)));
        KeReleaseSpinlock(&(Pic[1].mutex), prio);
    }

    return OK;
}

STATUS PicEnableIrq(uint32_t input)
{
    if(!CHECK_INPUT(input))
        return IT_BAD_VECTOR;

    if(IS_MASTER_INPUT(input))
    {
        PRIO prio = KeAcquireSpinlock(&(Pic[0].mutex));
        IoPortWriteByte(PIC_MASTER_DATA_PORT, IoPortReadByte(PIC_MASTER_DATA_PORT) & ~(1 << input));
        KeReleaseSpinlock(&(Pic[0].mutex), prio);
    }
    else
    {
        PRIO prio = KeAcquireSpinlock(&(Pic[1].mutex));
        IoPortWriteByte(PIC_SLAVE_DATA_PORT, IoPortReadByte(PIC_SLAVE_DATA_PORT) & ~(1 << (input - 8)));
        KeReleaseSpinlock(&(Pic[1].mutex), prio);
    }
    return OK;
}

void PicSetIrqMask(uint16_t mask)
{
    PRIO prio = KeAcquireSpinlock(&(Pic[0].mutex));
    prio = KeAcquireSpinlock(&(Pic[1].mutex));
    IoPortWriteByte(PIC_MASTER_DATA_PORT, mask & 0xFF);
    IoPortWriteByte(PIC_SLAVE_DATA_PORT, (mask >> 8) & 0xFF);
    KeReleaseSpinlock(&(Pic[1].mutex), prio);
    KeReleaseSpinlock(&(Pic[0].mutex), prio);
}

uint16_t PicGetIsr(void)
{
    PRIO prio = KeAcquireSpinlock(&(Pic[0].mutex));
    IoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_ISR);
    uint8_t t = IoPortReadByte(PIC_MASTER_CMD_PORT);
    KeReleaseSpinlock(&(Pic[0].mutex), prio);
    prio = KeAcquireSpinlock(&(Pic[1].mutex));
    IoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_ISR);
    uint16_t ret = ((uint16_t)(IoPortReadByte(PIC_SLAVE_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&(Pic[1].mutex), prio);
    return ret;
}

uint16_t PicGetIrr(void)
{
    PRIO prio = KeAcquireSpinlock(&(Pic[0].mutex));
    IoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_IRR);
    uint8_t t = IoPortReadByte(PIC_MASTER_CMD_PORT);
    KeReleaseSpinlock(&(Pic[0].mutex), prio);
    prio = KeAcquireSpinlock(&(Pic[1].mutex));
    IoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_IRR);
    uint16_t ret = ((uint16_t)(IoPortReadByte(PIC_SLAVE_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&(Pic[1].mutex), prio);
    return ret;
}

bool PicIsIrqSpurious(void)
{
    uint16_t isr = PicGetIsr();
    //a real interrupt results in setting a bit in the ISR register
    //check if there is any bit set. Omit bit 2 (bit 2 of master PIC), where slave PIC output is connected
    //if bit 2 is set, but no bit is set in slave PIC ISR, then the interrupt is spurious
    if(isr & 0xFB)
        return false;
    
    if(isr & 0x04) //spurious interrupt occured on slave PIC, so master PIC must be acknowledged
        PicSendEoi(0x04);

    return true;
}

uint32_t PicReserveInput(uint32_t input)
{
    if(HAL_INTERRUPT_INPUT_ANY == input)
    {
        for(uint8_t k = 0; k < 2; k++)
        {
            PRIO prio = KeAcquireSpinlock(&(Pic[k].mutex));
            for(uint8_t i = 0; i < PIC_INPUT_COUNT / 2; i++)
            {
                if(0 == (Pic[k].usage & (1 << i)))
                {
                    Pic[k].usage |= (1 << i);
                    KeReleaseSpinlock(&(Pic[k].mutex), prio);
                    return i;
                }
            }
            KeReleaseSpinlock(&(Pic[k].mutex), prio);
        }
        return UINT32_MAX;
    }
    else
    {
        uint8_t k;
        if(IS_MASTER_INPUT(input))
            k = 0;
        else if(IS_SLAVE_INPUT(input))
        {
            input -= 8;
            k = 1;
        }
        else
            return UINT32_MAX;
        
        PRIO prio = KeAcquireSpinlock(&(Pic[k].mutex));
        if(0 == (Pic[k].usage & (1 << input)))
        {
            Pic[k].usage |= (1 << input);
            KeReleaseSpinlock(&(Pic[k].mutex), prio);
            return input;
        }
        KeReleaseSpinlock(&(Pic[k].mutex), prio);
    }
    return UINT32_MAX;
}

void PicFreeInput(uint32_t input)
{
    uint8_t k;
    if(IS_MASTER_INPUT(input))
        k = 0;
    else if(IS_SLAVE_INPUT(input))
    {
        input -= 8;
        k = 1;
    }
    else
        return;
    
    PRIO prio = KeAcquireSpinlock(&(Pic[k].mutex));
    Pic[k].usage &= ~(1 << input);
    KeReleaseSpinlock(&(Pic[k].mutex), prio);
}

#endif