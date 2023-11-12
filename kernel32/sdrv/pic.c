#include "pic.h"
#include "hal/ioport.h"
#include "ke/core/mutex.h"

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

static uint8_t irqOffset[2] = {0, 0};

#define IS_MASTER_IRQ(x) (((x) >= irqOffset[0]) && ((x) <= (irqOffset[0] + 7)))
#define IS_SLAVE_IRQ(x) (((x) >= irqOffset[1]) && ((x) <= (irqOffset[1] + 7)))
#define CHECK_VECTOR(x) (IS_MASTER_IRQ(x) || IS_SLAVE_IRQ(x))

static KeSpinlock masterMutex = KeSpinlockInitializer, slaveMutex = KeSpinlockInitializer;

STATUS PicSendEOI(uint8_t irq)
{
    if(!CHECK_VECTOR(irq))
        return IT_BAD_VECTOR;
    
    if(IS_SLAVE_IRQ(irq))
    {
        KeAcquireSpinlock(&slaveMutex);
        PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_EOI);
        KeReleaseSpinlock(&slaveMutex);
    }

    KeAcquireSpinlock(&masterMutex);
    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_EOI);
    KeReleaseSpinlock(&masterMutex);
    return OK;
}

void PicRemap(uint8_t masterirqOffset, uint8_t slaveirqOffset)
{
    KeAcquireSpinlock(&masterMutex);
    uint8_t mMask = PortIoReadByte(PIC_MASTER_DATA_PORT);
    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, masterirqOffset);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, 4);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, PIC_ICW4_FLAG_8086);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, mMask);
    KeReleaseSpinlock(&masterMutex);

    KeAcquireSpinlock(&slaveMutex);
    uint8_t sMask = PortIoReadByte(PIC_SLAVE_DATA_PORT);
    PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, slaveirqOffset);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, 2);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, PIC_ICW4_FLAG_8086);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, sMask);
    KeReleaseSpinlock(&slaveMutex);

    irqOffset[0] = masterirqOffset;
    irqOffset[1] = slaveirqOffset;
}

STATUS PicDisableIRQ(uint8_t irq)
{
    if(!CHECK_VECTOR(irq))
        return IT_BAD_VECTOR;

    if(IS_MASTER_IRQ(irq))
    {
        KeAcquireSpinlock(&masterMutex);
        PortIoWriteByte(PIC_MASTER_DATA_PORT, PortIoReadByte(PIC_MASTER_DATA_PORT) | (1 << (irq - irqOffset[0])));
        KeReleaseSpinlock(&masterMutex);
    }
    else
    {
        KeAcquireSpinlock(&slaveMutex);
        PortIoWriteByte(PIC_MASTER_DATA_PORT, PortIoReadByte(PIC_SLAVE_DATA_PORT) | (1 << (irq - irqOffset[1])));
        KeReleaseSpinlock(&slaveMutex);
    }

    return OK;
}

STATUS PicEnableIRQ(uint8_t irq)
{
    if(!CHECK_VECTOR(irq))
        return IT_BAD_VECTOR;

    if(IS_MASTER_IRQ(irq))
    {
        KeAcquireSpinlock(&masterMutex);
        PortIoWriteByte(PIC_MASTER_DATA_PORT, PortIoReadByte(PIC_MASTER_DATA_PORT) & ~(1 << (irq - irqOffset[0])));
        KeReleaseSpinlock(&masterMutex);
    }
    else
    {
        KeAcquireSpinlock(&slaveMutex);
        PortIoWriteByte(PIC_SLAVE_DATA_PORT, PortIoReadByte(PIC_SLAVE_DATA_PORT) & ~(1 << (irq - irqOffset[1])));
        KeReleaseSpinlock(&slaveMutex);
    }
    return OK;
}

void PicSetIRQMask(uint16_t mask)
{
    KeAcquireSpinlock(&masterMutex);
    KeAcquireSpinlock(&slaveMutex);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, mask & 0xFF);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, (mask >> 8) & 0xFF);
    KeReleaseSpinlock(&slaveMutex);
    KeReleaseSpinlock(&masterMutex);
}

uint16_t PicGetISR(void)
{
    KeAcquireSpinlock(&masterMutex);
    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_ISR);
    uint8_t t = PortIoReadByte(PIC_MASTER_CMD_PORT);
    KeReleaseSpinlock(&masterMutex);
    KeAcquireSpinlock(&slaveMutex);
    PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_ISR);
    uint16_t ret = ((uint16_t)(PortIoReadByte(PIC_SLAVE_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&slaveMutex);
    return ret;
}

uint16_t PicGetIRR(void)
{
    KeAcquireSpinlock(&masterMutex);
    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_IRR);
    uint8_t t = PortIoReadByte(PIC_MASTER_CMD_PORT);
    KeReleaseSpinlock(&masterMutex);
    KeAcquireSpinlock(&slaveMutex);
    PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_IRR);
    uint16_t ret = ((uint16_t)(PortIoReadByte(PIC_SLAVE_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&slaveMutex);
    return ret;
}

bool PicIsIrqSpurious(void)
{
    uint16_t isr = PicGetISR();
    //a real interrupt results in setting a bit in the ISR register
    //check if there is any bit set. Omit bit 2 (bit 2 of master PIC), where slave PIC output is connected
    //if bit 2 is set, but no bit is set in slave PIC ISR, then the interrupt is spurious
    if(isr & 0xFB)
        return false;
    
    if(isr & 0x04) //spurious interrupt occured on slave PIC, so master PIC must be acknowledged
        PicSendEOI(0x04);

    return true;
}