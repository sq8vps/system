#if defined(__i686__) || defined(__amd64__)

#include "acpi.h"
#include "mm/dynmap.h"
#include "common.h"
#include "cpu.h"
#include "ioapic.h"
#include "irq.h"

#define ACPI_BDA_MEMORY_SIZE_LOCATION 0x413 //here the number of kilobytes between 0 and EBDA is stored
#define ACPI_BIOS_AREA_SPACE_LOCATION 0xE0000 //another alternative location of EBDA - 64 kiB BIOS memory
#define ACPI_BIOS_AREA_SPACE_SIZE 0x20000 //128 kiB

#define ACPI_MADT_ENTRY_LAPIC 0
#define ACPI_MADT_ENTRY_IOAPIC 1
#define ACPI_MADT_ENTRY_INTERRUPT_SOURCE 2
#define ACPI_MADT_ENTRY_NMI_SOURCE 3
#define ACPI_MADT_ENTRY_LAPIC_NMI_SOURCE 4
#define ACPI_MADT_ENTRY_LAPIC_OVERRIDE 5
#define ACPI_MADT_ENTRY_IOSAPIC 6
#define ACPI_MADT_ENTRY_LSAPIC 7
#define ACPI_MADT_ENTRY_PLATFORM_INTERRUPT 8
#define ACPI_MADT_ENTRY_LX2APIC 9

#define ACPI_LAPIC_ENTRY_FLAG_ENABLED 1
#define ACPI_LAPIC_ENTRY_FLAG_ONLINE_CAPABLE 2

struct AcpiRsdp
{
    char signature[8];
    uint8_t checksum;
    char oemId[6];
    uint8_t revision;
    uint32_t rsdtAddress;
} PACKED;

struct AcpiRsdpV2
{
    struct AcpiRsdp rsdp;
    uint32_t length;
    uint64_t xsdtAddress;
    uint8_t extendedChecksum;
    uint8_t reserved[3];
} PACKED;

struct AcpiRXsdt
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;
} PACKED;

#define ACPI_MADT_FLAG_PCAT_COMPAT 1

struct AcpiMadt
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemId[6];
    char oemTableId[8];
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;
    uint32_t lapicAddress;
    uint32_t flags;
} PACKED;

struct AcpiLapicEntry
{
    uint8_t type;
    uint8_t length;
    uint8_t cpuUid;
    uint8_t lapicId;
    uint32_t flags;
} PACKED;

struct AcpiIoApicEntry
{
    uint8_t type;
    uint8_t length;
    uint8_t ioApicId;
    uint8_t reserved;
    uint32_t ioApicAddress;
    uint32_t irqBase;
} PACKED;

struct AcpiIrqOverrideEntry
{
    uint8_t type;
    uint8_t length;
    uint8_t bus;
    uint8_t source;
    uint32_t globalIrq;
    uint16_t flags;
} PACKED;

static bool AcpiVerifyChecksum(const void *data, uint32_t size)
{
    uint8_t *pdata = (uint8_t*)data;
    uint8_t sum = 0;
    while(size--)
    {
        sum += *pdata++;
    }
    return (0 == sum);
}

static void *AcpiFindRsdp(void *in)
{
    for(uint16_t i = 0; i < MM_PAGE_SIZE; i += 16) 
    {
        if(0 == CmStrncmp(((const char*)in) + i, "RSD PTR ", 8))
        {
            return (void*)((uintptr_t)(in) + i);
        }
    }
    return NULL;
}

static uintptr_t AcpiGetRxsdtAddress(void)
{
    void *t = MmMapDynamicMemory(0, MM_LOWER_MEMORY_SIZE, 0); //map whole lower memory
    if(NULL == t)
        return 0;

    uintptr_t ebdaOffset = *((uint16_t*)((uintptr_t)t + ACPI_BDA_MEMORY_SIZE_LOCATION)) * 1024; //get EBDA offset in kilobytes and calculate address in bytes

    uint8_t *rsdp = NULL;

    //RSDP table pointer structure should lay within the 1st kB of EBDA on a 16-byte boundary
    if((0 == ebdaOffset) || (NULL == (rsdp = AcpiFindRsdp((void*)((uintptr_t)t + ebdaOffset)))))
    {
        //find in BIOS memory (64 kiB)
        for(uint16_t i = 0; i < (ACPI_BIOS_AREA_SPACE_SIZE / MM_PAGE_SIZE); i++)
        {
            if(NULL != (rsdp = AcpiFindRsdp((void*)((uintptr_t)t + ACPI_BIOS_AREA_SPACE_LOCATION + (i * MM_PAGE_SIZE)))))
                break;
        }
        if(NULL == rsdp)
        {
            MmUnmapDynamicMemory(t);
            return 0; //failure
        }
    }

    if(!AcpiVerifyChecksum(rsdp, sizeof(struct AcpiRsdp)))
    {
        MmUnmapDynamicMemory(t);
        return 0;
    }

    uintptr_t ret = 0;

    if(2 == ((struct AcpiRsdp*)rsdp)->revision) //revision 2, use XSDT
    {

        if(!AcpiVerifyChecksum(rsdp, sizeof(struct AcpiRsdpV2)))
        {
            MmUnmapDynamicMemory(t);
            return 0;
        }

        ret = ((struct AcpiRsdpV2*)rsdp)->xsdtAddress;
    }
    else //revision 1, use RSDT
    {
        ret = ((struct AcpiRsdp*)rsdp)->rsdtAddress;
    }


    MmUnmapDynamicMemory(t);
    return ret;
}

static void AcpiReadEntries(struct AcpiMadt *hdr)
{   
    void *entryLimit = ((uint8_t*)hdr + hdr->length);
    void *entry = ++hdr; //get first entry pointer

    while(entry < entryLimit)
    {
        switch(*((uint8_t*)entry))
        {
            case ACPI_MADT_ENTRY_LAPIC:
                struct AcpiLapicEntry *cpu = (struct AcpiLapicEntry*)entry;
                if(0 == (cpu->flags & (ACPI_LAPIC_ENTRY_FLAG_ENABLED | ACPI_LAPIC_ENTRY_FLAG_ONLINE_CAPABLE)))
                {
                    entry = (uint8_t*)entry + cpu->length;
                    break;
                }
                PRINT("CPU with APIC ID = 0x%X\n", cpu->lapicId);
                CpuAdd(cpu->lapicId, 
                    ((cpu->flags & ACPI_LAPIC_ENTRY_FLAG_ENABLED) || (cpu->flags & ACPI_LAPIC_ENTRY_FLAG_ONLINE_CAPABLE)),
                    !!(cpu->flags & ACPI_LAPIC_ENTRY_FLAG_ENABLED));
                entry = (uint8_t*)entry + cpu->length;
                break;
            case ACPI_MADT_ENTRY_IOAPIC:
                struct AcpiIoApicEntry *ioapic = (struct AcpiIoApicEntry*)entry;
                PRINT("I/O APIC with ID = 0x%X\n", ioapic->ioApicId);
                ApicIoAddEntry(ioapic->ioApicId, ioapic->ioApicAddress, ioapic->irqBase);
                entry = (uint8_t*)entry + ioapic->length;
                break;
            case ACPI_MADT_ENTRY_INTERRUPT_SOURCE:
                struct AcpiIrqOverrideEntry *irq = (struct AcpiIrqOverrideEntry*)entry;
                I686AddIsaRemapEntry(irq->source, irq->globalIrq);
                entry = (uint8_t*)entry + irq->length;
                break;
            default:
                entry = (uint8_t*)entry + ((uint8_t*)entry)[1];
                break;
        }
    }
}

STATUS AcpiInit(uintptr_t *lapicAddress)
{
    uintptr_t rsdtPhysical = AcpiGetRxsdtAddress();
    if(0 == rsdtPhysical)
        return ACPI_RSDT_NOT_FOUND;
    
    struct AcpiRXsdt *rxsdt = MmMapDynamicMemory(rsdtPhysical, sizeof(struct AcpiRXsdt), 0);
    if(NULL == rxsdt)
        return OUT_OF_RESOURCES;
    
    uint8_t entrySize = (0 == CmStrncmp(rxsdt->signature, "XSDT", 4)) ? 8 : 4;
    uint32_t entryCount = (rxsdt->length - sizeof(struct AcpiRXsdt)) / entrySize;
    uint32_t rxsdtLength = rxsdt->length;

    MmUnmapDynamicMemory(rxsdt);

    rxsdt = MmMapDynamicMemory(rsdtPhysical, rxsdtLength, 0);
    if(NULL == rxsdt)
        return OUT_OF_RESOURCES;

    if(!AcpiVerifyChecksum(rxsdt, rxsdtLength))
    {
        MmUnmapDynamicMemory(rxsdt);
        return ACPI_CHECKSUM_VALIDATION_FAILED;
    }

    for(uint32_t i = 0; i < entryCount; i++)
    {   
        uintptr_t madtAddress = *(uintptr_t*)(((uintptr_t)(rxsdt + 1)) + (i * entrySize));
        struct AcpiMadt *madt = MmMapDynamicMemory(madtAddress, sizeof(struct AcpiMadt), 0);
        if(NULL == madt)
        {
            MmUnmapDynamicMemory(rxsdt);
            return OUT_OF_RESOURCES;
        }
        if(0 != CmStrncmp(madt->signature, "APIC", 4))
        {
            MmUnmapDynamicMemory(madt);
            continue;
        }
        uint32_t madtSize = madt->length;
        MmUnmapDynamicMemory(madt);
        madt = MmMapDynamicMemory(madtAddress, madtSize, 0);
        if(NULL == madt)
        {
            MmUnmapDynamicMemory(rxsdt);
            return OUT_OF_RESOURCES;
        }

        if(!AcpiVerifyChecksum(madt, madtSize))
        {
            MmUnmapDynamicMemory(rxsdt);
            MmUnmapDynamicMemory(madt);
            return ACPI_CHECKSUM_VALIDATION_FAILED;
        }

        *lapicAddress = madt->lapicAddress;
        I686SetDualPicPresence((madt->flags & ACPI_MADT_FLAG_PCAT_COMPAT) > 0);

        AcpiReadEntries(madt);
        MmUnmapDynamicMemory(madt);
        return OK;
    }
    
    MmUnmapDynamicMemory(rxsdt);
    return ACPI_MADT_NOT_FOUND;

}

#endif