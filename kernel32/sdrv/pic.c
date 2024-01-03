#include "pic.h"
#include "hal/ioport.h"
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
        KeAcquireSpinlock(&(Pic[1].mutex));
        HalIoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_EOI);
        KeReleaseSpinlock(&(Pic[1].mutex));
    }

    KeAcquireSpinlock(&(Pic[0].mutex));
    HalIoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_EOI);
    KeReleaseSpinlock(&(Pic[0].mutex));
    return OK;
}

void PicRemap(uint8_t masterIrqOffset, uint8_t slaveIrqOffset)
{
    KeAcquireSpinlock(&(Pic[0].mutex));
    uint8_t mMask = HalIoPortReadByte(PIC_MASTER_DATA_PORT);
    HalIoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    HalIoPortWriteByte(PIC_MASTER_DATA_PORT, masterIrqOffset);
    HalIoPortWriteByte(PIC_MASTER_DATA_PORT, 4);
    HalIoPortWriteByte(PIC_MASTER_DATA_PORT, PIC_ICW4_FLAG_8086);
    HalIoPortWriteByte(PIC_MASTER_DATA_PORT, mMask);
    KeReleaseSpinlock(&(Pic[0].mutex));

    KeAcquireSpinlock(&(Pic[1].mutex));
    uint8_t sMask = HalIoPortReadByte(PIC_SLAVE_DATA_PORT);
    HalIoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    HalIoPortWriteByte(PIC_SLAVE_DATA_PORT, slaveIrqOffset);
    HalIoPortWriteByte(PIC_SLAVE_DATA_PORT, 2);
    HalIoPortWriteByte(PIC_SLAVE_DATA_PORT, PIC_ICW4_FLAG_8086);
    HalIoPortWriteByte(PIC_SLAVE_DATA_PORT, sMask);
    KeReleaseSpinlock(&(Pic[1].mutex));
}

STATUS PicDisableIrq(uint32_t input)
{
    if(!CHECK_INPUT(input))
        return IT_BAD_VECTOR;

    if(IS_MASTER_INPUT(input))
    {
        KeAcquireSpinlock(&(Pic[0].mutex));
        HalIoPortWriteByte(PIC_MASTER_DATA_PORT, HalIoPortReadByte(PIC_MASTER_DATA_PORT) | (1 << input));
        KeReleaseSpinlock(&(Pic[0].mutex));
    }
    else
    {
        KeAcquireSpinlock(&(Pic[1].mutex));
        HalIoPortWriteByte(PIC_MASTER_DATA_PORT, HalIoPortReadByte(PIC_SLAVE_DATA_PORT) | (1 << (input - 8)));
        KeReleaseSpinlock(&(Pic[1].mutex));
    }

    return OK;
}

STATUS PicEnableIrq(uint32_t input)
{
    if(!CHECK_INPUT(input))
        return IT_BAD_VECTOR;

    if(IS_MASTER_INPUT(input))
    {
        KeAcquireSpinlock(&(Pic[0].mutex));
        HalIoPortWriteByte(PIC_MASTER_DATA_PORT, HalIoPortReadByte(PIC_MASTER_DATA_PORT) & ~(1 << input));
        KeReleaseSpinlock(&(Pic[0].mutex));
    }
    else
    {
        KeAcquireSpinlock(&(Pic[1].mutex));
        HalIoPortWriteByte(PIC_SLAVE_DATA_PORT, HalIoPortReadByte(PIC_SLAVE_DATA_PORT) & ~(1 << (input - 8)));
        KeReleaseSpinlock(&(Pic[1].mutex));
    }
    return OK;
}

void PicSetIrqMask(uint16_t mask)
{
    KeAcquireSpinlock(&(Pic[0].mutex));
    KeAcquireSpinlock(&(Pic[1].mutex));
    HalIoPortWriteByte(PIC_MASTER_DATA_PORT, mask & 0xFF);
    HalIoPortWriteByte(PIC_SLAVE_DATA_PORT, (mask >> 8) & 0xFF);
    KeReleaseSpinlock(&(Pic[1].mutex));
    KeReleaseSpinlock(&(Pic[0].mutex));
}

uint16_t PicGetIsr(void)
{
    KeAcquireSpinlock(&(Pic[0].mutex));
    HalIoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_ISR);
    uint8_t t = HalIoPortReadByte(PIC_MASTER_CMD_PORT);
    KeReleaseSpinlock(&(Pic[0].mutex));
    KeAcquireSpinlock(&(Pic[1].mutex));
    HalIoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_ISR);
    uint16_t ret = ((uint16_t)(HalIoPortReadByte(PIC_SLAVE_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&(Pic[1].mutex));
    return ret;
}

uint16_t PicGetIrr(void)
{
    KeAcquireSpinlock(&(Pic[0].mutex));
    HalIoPortWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_IRR);
    uint8_t t = HalIoPortReadByte(PIC_MASTER_CMD_PORT);
    KeReleaseSpinlock(&(Pic[0].mutex));
    KeAcquireSpinlock(&(Pic[1].mutex));
    HalIoPortWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_IRR);
    uint16_t ret = ((uint16_t)(HalIoPortReadByte(PIC_SLAVE_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&(Pic[1].mutex));
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
            KeAcquireSpinlock(&(Pic[k].mutex));
            for(uint8_t i = 0; i < PIC_INPUT_COUNT / 2; i++)
            {
                if(0 == (Pic[k].usage & (1 << i)))
                {
                    Pic[k].usage |= (1 << i);
                    KeReleaseSpinlock(&(Pic[k].mutex));
                    return i;
                }
            }
            KeReleaseSpinlock(&(Pic[k].mutex));
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
        
        KeAcquireSpinlock(&(Pic[k].mutex));
        if(0 == (Pic[k].usage & (1 << input)))
        {
            Pic[k].usage |= (1 << input);
            KeReleaseSpinlock(&(Pic[k].mutex));
            return input;
        }
        KeReleaseSpinlock(&(Pic[k].mutex));
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
    
    KeAcquireSpinlock(&(Pic[k].mutex));
    Pic[k].usage &= ~(1 << input);
    KeReleaseSpinlock(&(Pic[k].mutex));
}