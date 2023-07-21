#include "acpi.h"
#include "mm/dynmap.h"
#include "common.h"
#include "hal/cpu.h"
#include "ioapic.h"

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
    uint32_t globalIRQ;
} PACKED;

static bool verifyChecksum(const void *data, uint32_t size)
{
    uint8_t *pdata = (uint8_t*)data;
    uint8_t sum = 0;
    while(size--)
    {
        sum += *pdata++;
    }
    return (0 == sum);
}

static void *findRSDP(void *in)
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

static uintptr_t getRXSDTAddress(void)
{
    void *t = MmMapDynamicMemory(0, MM_LOWER_MEMORY_SIZE, 0); //map whole lower memory
    if(NULL == t)
        return 0;

    uintptr_t ebdaOffset = *((uint16_t*)((uintptr_t)t + ACPI_BDA_MEMORY_SIZE_LOCATION)) * 1024; //get EBDA offset in kilobytes and calculate address in bytes

    uint8_t *rsdp = NULL;

    //RSDP table pointer structure should lay within the 1st kB of EBDA on a 16-byte boundary
    if((0 == ebdaOffset) || (NULL == (rsdp = findRSDP((void*)((uintptr_t)t + ebdaOffset)))))
    {
        //find in BIOS memory (64 kiB)
        for(uint16_t i = 0; i < (ACPI_BIOS_AREA_SPACE_SIZE / MM_PAGE_SIZE); i++)
        {
            if(NULL != (rsdp = findRSDP((void*)((uintptr_t)t + ACPI_BIOS_AREA_SPACE_LOCATION + (i * MM_PAGE_SIZE)))))
                break;
        }
        if(NULL == rsdp)
        {
            MmUnmapDynamicMemory(t, MM_LOWER_MEMORY_SIZE);
            return 0; //failure
        }
    }

    if(!verifyChecksum(rsdp, sizeof(struct AcpiRsdp)))
    {
        MmUnmapDynamicMemory(t, MM_LOWER_MEMORY_SIZE);
        return 0;
    }

    uintptr_t ret = 0;

    if(2 == ((struct AcpiRsdp*)rsdp)->revision) //revision 2, use XSDT
    {

        if(!verifyChecksum(rsdp, sizeof(struct AcpiRsdpV2)))
        {
            MmUnmapDynamicMemory(t, MM_LOWER_MEMORY_SIZE);
            return 0;
        }

        ret = ((struct AcpiRsdpV2*)rsdp)->xsdtAddress;
    }
    else //revision 1, use RSDT
    {
        ret = ((struct AcpiRsdp*)rsdp)->rsdtAddress;
    }


    MmUnmapDynamicMemory(t, MM_LOWER_MEMORY_SIZE);
    return ret;
}

static void readEntries(struct AcpiMadt *hdr)
{   
    void *entryLimit = ((uint8_t*)hdr + hdr->length);
    void *entry = ++hdr; //get first entry pointer

    while(entry < entryLimit)
    {
        switch(*((uint8_t*)entry))
        {
            case ACPI_MADT_ENTRY_LAPIC:
                if(HalCpuCount >= MAX_CPU_COUNT)
                    break;
                struct AcpiLapicEntry *cpu = (struct AcpiLapicEntry*)entry;
                if(0 == (cpu->flags & (ACPI_LAPIC_ENTRY_FLAG_ENABLED | ACPI_LAPIC_ENTRY_FLAG_ONLINE_CAPABLE)))
                    break;
                HalCpuConfigTable[HalCpuCount].apicId = cpu->lapicId;

                HalCpuConfigTable[HalCpuCount].boot = (cpu->flags & ACPI_LAPIC_ENTRY_FLAG_ENABLED) > 0;
                HalCpuConfigTable[HalCpuCount].usable = ((cpu->flags & ACPI_LAPIC_ENTRY_FLAG_ENABLED) > 0) 
                                                        | ((cpu->flags & ACPI_LAPIC_ENTRY_FLAG_ONLINE_CAPABLE) > 0);
                HalCpuCount++;
                entry = (uint8_t*)entry + cpu->length;
                break;
            case ACPI_MADT_ENTRY_IOAPIC:
                if(ApicIoEntryCount >= MAX_IOAPIC_COUNT)
                    break;
                struct AcpiIoApicEntry *ioapic = (struct AcpiIoApicEntry*)entry;
                ApicIoEntryTable[ApicIoEntryCount].id = ioapic->ioApicId;
                ApicIoEntryTable[ApicIoEntryCount].address = ioapic->ioApicAddress;
                ApicIoEntryCount++;
                entry = (uint8_t*)entry + ioapic->length;
                break;
            default:
                entry = (uint8_t*)entry + ((uint8_t*)entry)[1];
                break;
        }
    }
}

STATUS AcpiInit(uintptr_t *lapicAddress)
{
    uintptr_t rsdtPhysical = getRXSDTAddress();
    if(0 == rsdtPhysical)
        RETURN(ACPI_RSDT_NOT_FOUND);
    
    struct AcpiRXsdt *rxsdt = MmMapDynamicMemory(rsdtPhysical, sizeof(struct AcpiRXsdt), 0);
    if(NULL == rxsdt)
        RETURN(MM_DYNAMIC_MEMORY_ALLOCATION_FAILURE);
    
    uint8_t entrySize = (0 == CmStrncmp(rxsdt->signature, "XSDT", 4)) ? 8 : 4;
    uint32_t entryCount = (rxsdt->length - sizeof(struct AcpiRXsdt)) / entrySize;
    uint32_t rxsdtLength = rxsdt->length;

    MmUnmapDynamicMemory(rxsdt, sizeof(struct AcpiRXsdt));

    rxsdt = MmMapDynamicMemory(rsdtPhysical, rxsdtLength, 0);
    if(NULL == rxsdt)
        RETURN(MM_DYNAMIC_MEMORY_ALLOCATION_FAILURE);

    if(!verifyChecksum(rxsdt, rxsdtLength))
    {
        MmUnmapDynamicMemory(rxsdt, rxsdtLength);
        RETURN(ACPI_CHECKSUM_VALIDATION_FAILED);
    }

    for(uint32_t i = 0; i < entryCount; i++)
    {   
        uintptr_t madtAddress = *(uintptr_t*)(((uintptr_t)(rxsdt + 1)) + (i * entrySize));
        struct AcpiMadt *madt = MmMapDynamicMemory(madtAddress, sizeof(struct AcpiMadt), 0);
        if(NULL == madt)
        {
            MmUnmapDynamicMemory(rxsdt, rxsdtLength);
            RETURN(MM_DYNAMIC_MEMORY_ALLOCATION_FAILURE);
        }
        if(0 != CmStrncmp(madt->signature, "APIC", 4))
        {
            MmUnmapDynamicMemory(madt, sizeof(struct AcpiMadt));
            continue;
        }
        uint32_t madtSize = madt->length;
        MmUnmapDynamicMemory(madt, sizeof(struct AcpiMadt));
        madt = MmMapDynamicMemory(madtAddress, madtSize, 0);
        if(NULL == madt)
        {
            MmUnmapDynamicMemory(rxsdt, rxsdtLength);
            RETURN(MM_DYNAMIC_MEMORY_ALLOCATION_FAILURE);
        }

        if(!verifyChecksum(madt, madtSize))
        {
            MmUnmapDynamicMemory(rxsdt, rxsdtLength);
            MmUnmapDynamicMemory(madt, madtSize);
            RETURN(ACPI_CHECKSUM_VALIDATION_FAILED);
        }

        *lapicAddress = madt->lapicAddress;

        readEntries(madt);
        MmUnmapDynamicMemory(madt, madtSize);
        return OK;
    }
    
    MmUnmapDynamicMemory(rxsdt, rxsdtLength);
    RETURN(ACPI_MADT_NOT_FOUND);

}