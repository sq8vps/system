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

static KeSpinlock eoiMutex  = KeSpinlockInitializer, remapMutex = KeSpinlockInitializer,
 disableMutex = KeSpinlockInitializer, enableMutex = KeSpinlockInitializer, 
 setMaskMutex = KeSpinlockInitializer, getISRMutex = KeSpinlockInitializer, getIRRMutex = KeSpinlockInitializer;

STATUS PicSendEOI(uint8_t irq)
{
    if(irq > 15)
        return IT_BAD_VECTOR;
    
    KeAcquireSpinlock(&eoiMutex);

    if(irq >= 8)
        PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_EOI);

    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_EOI);
    KeReleaseSpinlock(&eoiMutex);
    return OK;
}

void PicRemap(uint8_t masterOffset, uint8_t slaveOffset)
{
    KeAcquireSpinlock(&remapMutex);

    uint8_t mMask = PortIoReadByte(PIC_MASTER_DATA_PORT);
    uint8_t sMask = PortIoReadByte(PIC_SLAVE_DATA_PORT);

    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_ICW1_FLAG_IC4 | PIC_ICW1_FLAG_INIT);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, masterOffset);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, slaveOffset);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, 4);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, 2);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, PIC_ICW4_FLAG_8086);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, PIC_ICW4_FLAG_8086);
    
    PortIoWriteByte(PIC_MASTER_DATA_PORT, mMask);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, sMask);

    KeReleaseSpinlock(&remapMutex);
}

STATUS PicDisableIRQ(uint8_t irq)
{
    if(irq > 15)
        return IT_BAD_VECTOR;

    KeAcquireSpinlock(&disableMutex);

    if(irq < 8)
    {
        PortIoWriteByte(PIC_MASTER_DATA_PORT, PortIoReadByte(PIC_MASTER_DATA_PORT) | (1 << irq));
    }
    else
    {
        PortIoWriteByte(PIC_MASTER_DATA_PORT, PortIoReadByte(PIC_SLAVE_DATA_PORT) | (1 << (irq - 8)));
    }

    KeReleaseSpinlock(&disableMutex);
    return OK;
}

STATUS PicEnableIRQ(uint8_t irq)
{
    if(irq > 15)
        return IT_BAD_VECTOR;

    KeAcquireSpinlock(&enableMutex);
    if(irq < 8)
    {
        PortIoWriteByte(PIC_MASTER_DATA_PORT, PortIoReadByte(PIC_MASTER_DATA_PORT) & ~(1 << irq));
    }
    else
    {
        PortIoWriteByte(PIC_SLAVE_DATA_PORT, PortIoReadByte(PIC_SLAVE_DATA_PORT) & ~(1 << (irq - 8)));
    }
    KeReleaseSpinlock(&enableMutex);
    return OK;
}

void PicSetIRQMask(uint16_t mask)
{
    KeAcquireSpinlock(&setMaskMutex);
    PortIoWriteByte(PIC_MASTER_DATA_PORT, mask & 0xFF);
    PortIoWriteByte(PIC_SLAVE_DATA_PORT, (mask >> 8) & 0xFF);
    KeReleaseSpinlock(&setMaskMutex);
}

uint16_t PicGetISR(void)
{
    KeAcquireSpinlock(&getISRMutex);
    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_ISR);
    uint8_t t = PortIoReadByte(PIC_MASTER_CMD_PORT);
    PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_ISR);
    uint16_t ret = ((uint16_t)(PortIoReadByte(PIC_MASTER_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&getISRMutex);
    return ret;
}

uint16_t PicGetIRR(void)
{
    KeAcquireSpinlock(&getIRRMutex);
    PortIoWriteByte(PIC_MASTER_CMD_PORT, PIC_CMD_READ_IRR);
    uint8_t t = PortIoReadByte(PIC_MASTER_CMD_PORT);
    PortIoWriteByte(PIC_SLAVE_CMD_PORT, PIC_CMD_READ_IRR);
    uint16_t ret = ((uint16_t)(PortIoReadByte(PIC_MASTER_CMD_PORT)) << 8) | (uint16_t)t;
    KeReleaseSpinlock(&getIRRMutex);
    return ret;
}

bool PicCheckSpuriousIRQ7(void)
{
    if(PicGetISR() & 0x08) //check if interrupt was real
        return false;
    
    return true;
}

bool PicCheckSpuriousIRQ15(void)
{
    if(PicGetISR() & 0x80) //check if interrupt was real
        return false;
    
    //not real?
    PicSendEOI(0); //send EOI to master PIC (any interrupt between 0 and 7)
    return true;
}