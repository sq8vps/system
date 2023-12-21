#include "ioapic.h"
#include "mm/mmio.h"
#include "it/it.h"
#include "common.h"

struct ApicIoEntry ApicIoEntryTable[MAX_IOAPIC_COUNT];
uint8_t ApicIoEntryCount = 0;

#define IOAPIC_IOREGSEL_OFFSET 0
#define IOAPIC_IOWIN_OFFSET 0x10

#define IOAPIC_REG_IOAPICID 0x00
#define IOAPIC_REG_IOAPICVER 0x01
#define IOAPIC_REG_APICARB 0x02
#define IOAPIC_REG_IOREDTBL(n) (0x10 + 2 * (n))

#define IOAPIC_IOREDTBL_VECTOR_MASK 0xFF
#define IOAPIC_IOREDTBL_MASK (1 << 16)


struct IOAPIC
{
    uint32_t *mmio;
    uint8_t id;
    uint8_t inputs;
    uint32_t irqBase;
} static ioApicDevice[MAX_IOAPIC_COUNT]; //table of I/O APIC chips
static uint8_t ioApicDeviceCount = 0; //number of I/O APIC chips

struct
{
    struct IOAPIC *ioApic;
    uint32_t input;
} static ioApicIrqLUT[256]; //look-up table to speed up getting IOAPIC structure and input number for given IRQ

static struct IOAPIC* findIoApic(uint32_t input)
{
    for(uint8_t i = 0; i < ioApicDeviceCount; i++)
    {
        if((ioApicDevice[i].irqBase < input) && ((ioApicDevice[i].irqBase + ioApicDevice[i].inputs) >= input))
            return &ioApicDevice[i];
    }
    return NULL;
}

static void write(struct IOAPIC *ioapic, uint8_t reg, uint32_t val)
{
    ioapic->mmio[IOAPIC_IOREGSEL_OFFSET] = reg;
    ioapic->mmio[IOAPIC_IOWIN_OFFSET / sizeof(*(ioapic->mmio))] = val;
}

static void write64(struct IOAPIC *ioapic, uint8_t reg, uint64_t val)
{
    write(ioapic, reg, val & 0xFFFFFFFF);
    write(ioapic, reg + 1, val >> 32);
}

static uint32_t read(struct IOAPIC *ioapic, uint8_t reg)
{
    ioapic->mmio[IOAPIC_IOREGSEL_OFFSET] = reg;
    return ioapic->mmio[IOAPIC_IOWIN_OFFSET / sizeof(*(ioapic->mmio))];
}

static uint64_t read64(struct IOAPIC *ioapic, uint8_t reg)
{
    uint64_t ret = read(ioapic, reg + 1);
    ret <<= 32;
    ret |= read(ioapic, reg);
    return ret;
}

STATUS ApicIoInit(void)
{
    if(0 == ApicIoEntryCount)
        return APIC_IOAPIC_NOT_AVAILABLE;

    CmMemset(ioApicIrqLUT, 0, sizeof(ioApicIrqLUT));

    uintptr_t lowestAddress = 0xFFFFFFFF;
    uintptr_t highestAddress = 0;
    //find lowest and highest physical address first
    for(uint16_t i = 0; i < ApicIoEntryCount; i++)
    {
        if(ApicIoEntryTable[i].address < lowestAddress)
            lowestAddress = ApicIoEntryTable[i].address;
        else if(ApicIoEntryTable[i].address > highestAddress)
            highestAddress = ApicIoEntryTable[i].address;
    }

    uint32_t *mmio = MmMapMmIo(lowestAddress, (highestAddress - lowestAddress + 32));
    if(NULL == mmio)
        return OUT_OF_RESOURCES;

    for(uint16_t i = 0; i < ApicIoEntryCount; i++)
    {
        ioApicDevice[ioApicDeviceCount].mmio = (uint32_t*)(((uintptr_t)mmio) + (ApicIoEntryTable[i].address - lowestAddress));
        ioApicDevice[ioApicDeviceCount].inputs = (read(&ioApicDevice[ioApicDeviceCount], IOAPIC_REG_IOAPICVER) >> 16) & 0x7F;
        ioApicDevice[ioApicDeviceCount].id = ApicIoEntryTable[i].id;
        ioApicDevice[ioApicDeviceCount].irqBase = ApicIoEntryTable[i].irqBase;
        ioApicDeviceCount++;
    }

    return OK;
}

uint8_t ApicIoGetAssociatedVector(uint32_t input)
{    
    for(uint16_t i = 0; i < 256; i++)
    {
        if(ioApicIrqLUT[i].input == input)
            return i;
    }
    return 0;
}

STATUS ApicIoRegisterIRQ(uint32_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger)
{
    struct IOAPIC *ioApic = findIoApic(input);
    if(NULL == ioApic)
        return APIC_IOAPIC_NOT_AVAILABLE;
    
    if(input < ioApic->irqBase)
        return APIC_IOAPIC_BAD_INPUT_NUMBER;
    
    if((input - ioApic->irqBase) >= ioApic->inputs)
        return APIC_IOAPIC_BAD_INPUT_NUMBER;

    if(vector < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    
    if(NULL != ioApicIrqLUT[ApicIoGetAssociatedVector(input)].ioApic)
        return IT_ALREADY_REGISTERED;
    
    write64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase), 
        ((uint32_t)vector) | (((uint32_t)mode) << 8) | (((uint32_t)polarity) << 13) | (((uint32_t)trigger) << 15) | IOAPIC_IOREDTBL_MASK);

    ioApicIrqLUT[vector].input = input;
    ioApicIrqLUT[vector].ioApic = ioApic;

    return OK;
}

STATUS ApicIoUnregisterIRQ(uint32_t input)
{
    struct IOAPIC *ioApic = findIoApic(input);
    if(NULL == ioApic)
        return APIC_IOAPIC_NOT_AVAILABLE;
    
    if(input < ioApic->irqBase)
        return APIC_IOAPIC_BAD_INPUT_NUMBER;
    
    if((input - ioApic->irqBase) >= ioApic->inputs)
        return APIC_IOAPIC_BAD_INPUT_NUMBER;

    uint8_t vector = ApicIoGetAssociatedVector(input);
    if(NULL == ioApicIrqLUT[vector].ioApic)
        return OK;
    
    write64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase), IT_FIRST_INTERRUPT_VECTOR | IOAPIC_IOREDTBL_MASK);

    ioApicIrqLUT[vector].input = 0;
    ioApicIrqLUT[vector].ioApic = NULL;

    return OK;
}



STATUS ApicIoEnableIRQ(uint8_t vector)
{
    struct IOAPIC *ioapic = ioApicIrqLUT[vector].ioApic;
    if(NULL == ioapic)
        return IT_NOT_REGISTERED;
    
    if(ioApicIrqLUT[vector].input < ioapic->irqBase)
        return IT_BAD_VECTOR;
    
    if(ioApicIrqLUT[vector].input >= (ioapic->irqBase + ioapic->inputs))
        return IT_BAD_VECTOR;
    
    write64(ioApicIrqLUT[vector].ioApic, IOAPIC_REG_IOREDTBL(ioApicIrqLUT[vector].input - ioapic->irqBase),
            read64(ioApicIrqLUT[vector].ioApic, IOAPIC_REG_IOREDTBL(ioApicIrqLUT[vector].input - ioapic->irqBase) & ~(IOAPIC_IOREDTBL_MASK)));
    
    return OK;
}

STATUS ApicIoDisableIRQ(uint8_t vector)
{
    struct IOAPIC *ioapic = ioApicIrqLUT[vector].ioApic;
    if(NULL == ioapic)
        return IT_NOT_REGISTERED;
    
    if(ioApicIrqLUT[vector].input < ioapic->irqBase)
        return IT_BAD_VECTOR;

    if(ioApicIrqLUT[vector].input >= (ioapic->irqBase + ioapic->inputs))
        return IT_BAD_VECTOR;
    
    write64(ioApicIrqLUT[vector].ioApic, IOAPIC_REG_IOREDTBL(ioApicIrqLUT[vector].input - ioapic->irqBase),
            read64(ioApicIrqLUT[vector].ioApic, IOAPIC_REG_IOREDTBL(ioApicIrqLUT[vector].input - ioapic->irqBase) | IOAPIC_IOREDTBL_MASK));
    
    return OK;
}