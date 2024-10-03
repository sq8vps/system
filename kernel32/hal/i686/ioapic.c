#if defined(__i686__) || defined(__amd64__)

#include "ioapic.h"
#include "mm/mmio.h"
#include "it/it.h"
#include "common.h"
#include "ke/core/mutex.h"

#define MAX_IOAPIC_COUNT 4

struct IoApicEntry
{
   uint8_t id;
   uint32_t address;
   uint32_t irqBase;
};

struct IoApicEntry IoApicEntryTable[MAX_IOAPIC_COUNT];
uint8_t IoApicEntryCount = 0;

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
    volatile uint32_t *mmio;
    uint8_t id;
    uint8_t inputs;
    uint32_t irqBase;
    uint32_t usage[(IOAPIC_MAX_INPUTS / 32) + ((IOAPIC_MAX_INPUTS % 32) ? 1 : 0)];
    KeSpinlock Lock;
} static IoApicDevice[MAX_IOAPIC_COUNT]; //table of I/O APIC chips
static uint8_t IoApicDeviceCount = 0; //number of I/O APIC chips

void ApicIoAddEntry(uint8_t id, uint32_t address, uint32_t irqBase)
{
    if(IoApicEntryCount >= MAX_IOAPIC_COUNT)
        return;
    
    IoApicEntryTable[IoApicEntryCount].id = id;
    IoApicEntryTable[IoApicEntryCount].address = address;
    IoApicEntryTable[IoApicEntryCount].irqBase = irqBase;
    IoApicEntryCount++;
}

static struct IOAPIC* findIoApic(uint32_t input)
{
    for(uint8_t i = 0; i < IoApicDeviceCount; i++)
    {
        PRIO prio = KeAcquireSpinlock(&IoApicDevice[i].Lock);
        if((IoApicDevice[i].irqBase <= input) && ((IoApicDevice[i].irqBase + IoApicDevice[i].inputs) >= input))
        {
            KeReleaseSpinlock(&IoApicDevice[i].Lock, prio);
            return &IoApicDevice[i];
        }
        KeReleaseSpinlock(&IoApicDevice[i].Lock, prio);
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
    if(0 == IoApicEntryCount)
        return APIC_IOAPIC_NOT_AVAILABLE;

    uintptr_t lowestAddress = 0xFFFFFFFF;
    uintptr_t highestAddress = 0;
    //find lowest and highest physical address first
    for(uint16_t i = 0; i < IoApicEntryCount; i++)
    {
        if(IoApicEntryTable[i].address < lowestAddress)
            lowestAddress = IoApicEntryTable[i].address;
        if(IoApicEntryTable[i].address > highestAddress)
            highestAddress = IoApicEntryTable[i].address;
    }

    volatile uint32_t *mmio = MmMapMmIo(lowestAddress, (highestAddress - lowestAddress + 32));
    if(NULL == mmio)
        return OUT_OF_RESOURCES;

    for(uint16_t i = 0; i < IoApicEntryCount; i++)
    {
        PRIO prio = KeAcquireSpinlock(&IoApicDevice[IoApicDeviceCount].Lock);
        IoApicDevice[IoApicDeviceCount].mmio = (uint32_t*)(((uintptr_t)mmio) + (IoApicEntryTable[i].address - lowestAddress));
        IoApicDevice[IoApicDeviceCount].inputs = (read(&IoApicDevice[IoApicDeviceCount], IOAPIC_REG_IOAPICVER) >> 16) & 0x7F;
        IoApicDevice[IoApicDeviceCount].id = IoApicEntryTable[i].id;
        IoApicDevice[IoApicDeviceCount].irqBase = IoApicEntryTable[i].irqBase;
        CmMemset(IoApicDevice[IoApicDeviceCount].usage, 0, sizeof(IoApicDevice[IoApicDeviceCount].usage));
        KeReleaseSpinlock(&IoApicDevice[IoApicDeviceCount].Lock, prio);
        IoApicDeviceCount++;
    }

    return OK;
}

uint8_t ApicIoGetAssociatedVector(uint32_t input)
{    
    struct IOAPIC *ioApic = findIoApic(input);
    if(NULL == ioApic)
        return 0;
    PRIO prio = KeAcquireSpinlock(&ioApic->Lock);
    uint8_t vector = read64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase)) & IOAPIC_IOREDTBL_VECTOR_MASK;
    KeReleaseSpinlock(&ioApic->Lock, prio);
    return vector;
}

STATUS ApicIoRegisterIrq(uint32_t input, uint8_t vector, enum HalInterruptMode mode,
                        enum HalInterruptPolarity polarity, enum HalInterruptTrigger trigger)
{
    struct IOAPIC *ioApic = findIoApic(input);
    if(NULL == ioApic)
        return APIC_IOAPIC_NOT_AVAILABLE;

    if(vector < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    uint64_t params = 0;
    switch(mode)
    {
        case IT_MODE_LOWEST_PRIORITY:
            params |= (0b001 << 8);
            break;
        case IT_MODE_SMI:
            params |= (0b010 << 8);
            break;
        case IT_MODE_NMI:
            params |= (0b100 << 8);
            break;
        case IT_MODE_INIT:
            params |= (0b101 << 8);
            break;
        case IT_MODE_EXTINT:
            params |= (0b111 << 8);
        case IT_MODE_FIXED:
        default:
            break;
    }
    switch(polarity)
    {
        case IT_POLARITY_ACTIVE_LOW:
            params |= (1 << 13);
            break;
        case IT_POLARITY_ACTIVE_HIGH:
        default:
            break;
    }
    switch(trigger)
    {
        case IT_TRIGGER_LEVEL:
            params |= (1 << 15);
            break;
        case IT_TRIGGER_EDGE:
        default:
            break;
    }
    params |= vector;
    params &= ~IOAPIC_IOREDTBL_MASK;
    PRIO prio = KeAcquireSpinlock(&ioApic->Lock);
    write64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase), params);
    KeReleaseSpinlock(&ioApic->Lock, prio);

    return OK;
}

STATUS ApicIoUnregisterIrq(uint32_t input)
{
    struct IOAPIC *ioApic = findIoApic(input);
    if(NULL == ioApic)
        return APIC_IOAPIC_NOT_AVAILABLE;
    
    PRIO prio = KeAcquireSpinlock(&ioApic->Lock);
    write64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase), IT_FIRST_INTERRUPT_VECTOR | IOAPIC_IOREDTBL_MASK);
    KeReleaseSpinlock(&ioApic->Lock, prio);

    return OK;
}

STATUS ApicIoEnableIrq(uint32_t input)
{
    struct IOAPIC *ioApic = findIoApic(input);
    if(NULL == ioApic)
        return IT_NOT_REGISTERED;
    
    PRIO prio = KeAcquireSpinlock(&ioApic->Lock);
    write64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase),
            read64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase) & ~(IOAPIC_IOREDTBL_MASK)));
    KeReleaseSpinlock(&ioApic->Lock, prio);
    
    return OK;
}

STATUS ApicIoDisableIrq(uint32_t input)
{
    struct IOAPIC *ioApic = findIoApic(input);
    if(NULL == ioApic)
        return IT_NOT_REGISTERED;
    
    PRIO prio = KeAcquireSpinlock(&ioApic->Lock);
    write64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase),
            read64(ioApic, IOAPIC_REG_IOREDTBL(input - ioApic->irqBase) | IOAPIC_IOREDTBL_MASK));
    KeReleaseSpinlock(&ioApic->Lock, prio);
    
    return OK;
}

uint32_t ApicIoReserveInput(uint32_t input)
{
    for(uint8_t i = 0; i < IoApicDeviceCount; i++)
    {
        PRIO prio = KeAcquireSpinlock(&IoApicDevice[i].Lock);
        if(HAL_INTERRUPT_INPUT_ANY == input)
        {
            for(uint16_t k = 0; k < IoApicDevice[i].inputs; k++)
            {
                if(0 == (IoApicDevice[i].usage[k / 32] & ((uint32_t)1 << (k % 32))))
                {
                    IoApicDevice[i].usage[k / 32] |= ((uint32_t)1 << (k % 32));
                    input = k + IoApicDevice[i].irqBase;
                    KeReleaseSpinlock(&IoApicDevice[i].Lock, prio);
                    return input;
                }
            }
        }
        else if((input >= IoApicDevice[i].irqBase) && (input < (IoApicDevice[i].irqBase + IoApicDevice[i].inputs)))
        {
            input -= IoApicDevice[i].irqBase;
            if(0 == (IoApicDevice[i].usage[input / 32] & ((uint32_t)1 << (input % 32))))
            {
                IoApicDevice[i].usage[input / 32] |= (uint32_t)1 << (input % 32);
                input += IoApicDevice[i].irqBase;
            }
            else
                input = UINT32_MAX;
            KeReleaseSpinlock(&IoApicDevice[i].Lock, prio);
            return UINT32_MAX;
        }
        KeReleaseSpinlock(&IoApicDevice[i].Lock, prio);
    }
    return UINT32_MAX;
}

void ApicIoFreeInput(uint32_t input)
{
    for(uint8_t i = 0; i < IoApicDeviceCount; i++)
    {
        PRIO prio = KeAcquireSpinlock(&IoApicDevice[i].Lock);
        if((input >= IoApicDevice[i].irqBase) && (input < (IoApicDevice[i].irqBase + IoApicDevice[i].inputs)))
        {
            input -= IoApicDevice[i].irqBase;
            IoApicDevice[i].usage[input / 32] &= ~((uint32_t)1 << (input % 32));
            KeReleaseSpinlock(&IoApicDevice[i].Lock, prio);
            break;
        }
        KeReleaseSpinlock(&IoApicDevice[i].Lock, prio);
    }
}

#endif