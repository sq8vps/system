#include "mm/dynmap.h"
#include "rtl/string.h"
#include "defines.h"
#include "hal/i686/memory.h"
#include "logging.h"

#define MP_BDA_MEMORY_SIZE_LOCATION 0x413 //here the number of kilobytes between 0 and EBDA is stored
#define MP_LOWER_MEMORY_LOCATION 0x9FC00 //here is the alternative location of EBDA
#define MP_BIOS_AREA_SPACE_LOCATION 0xF0000 //another alternative location of EBDA - 64 kiB BIOS memory
#define MP_BIOS_AREA_SPACE_SIZE 0x10000 //64 kiB

#define MP_FPS_SIZE 16 //floating pointer structure size
#define MP_FPS_CONFIGURATION_TABLE_ADDRESS_OFFSET 1

#define MP_FLOATING_POINTER_STRUCTURE_SIGNATURE "_MP_"
#define MP_CONFIGURATION_TABLE_HEADER_SIGNATURE "PCMP"
#define MP_CTH_SIZE 44 //configuration table header size

#define MP_CONFIGURATION_TABLE_ENTRY_CPU 0
#define MP_CONFIGURATION_TABLE_ENTRY_BUS 1
#define MP_CONFIGURATION_TABLE_ENTRY_IOAPIC 2
#define MP_CONFIGURATION_TABLE_ENTRY_IO_INTERRUPT 3
#define MP_CONFIGURATION_TABLE_ENTRY_LOCAL_INTERRUPT 4

#define MP_CPU_ENTRY_FLAG_EN 1
#define MP_CPU_ENTRY_FLAG_BP 2

#define MP_IOAPIC_ENTRY_FLAG_EN 1

struct MpConfigurationTableHeader
{
    char signature[4];
    uint16_t size;
    uint8_t revision;
    uint8_t checksum;
    char oem[8];
    char product[12];
    uint32_t oemTablePointer;
    uint16_t oemTableSize;
    uint16_t entryCount;
    uint32_t localApicAddress;
    uint16_t extendedTableSize;
    uint8_t extendedTableChecksum;
    uint8_t reserved;
} PACKED;

struct MpProcessorEntry
{
    uint8_t entryType;
    uint8_t localApicId;
    uint8_t localApicVersion;
    uint8_t cpuFlags;
    uint32_t cpuSignature;
    uint32_t featureFlags;
    uint64_t reserved;
} PACKED;

struct MpBusEntry
{
    uint8_t entryType;
    uint8_t busId;
    char busType[6];
} PACKED;

struct MpIOAPICEntry
{
    uint8_t entryType;
    uint8_t ioApicId;
    uint8_t ioApicVersion;
    uint8_t ioApicFlags;
    uint32_t ioApicAddress;
} PACKED;

struct MpIOInterruptAssignmentEntry
{
    uint8_t entryType;
    uint8_t interruptType;
    uint16_t ioInterruptFlags;
    uint8_t sourceBusId;
    uint8_t sourceBusIrq;
    uint8_t destinationIoApicId;
    uint8_t destinationIoApicIntin;
} PACKED;

struct MpLocalInterruptAssignmentEntry
{
    uint8_t entryType;
    uint8_t interruptType;
    uint16_t localInterruptFlags;
    uint8_t sourceBusId;
    uint8_t sourceBusIrq;
    uint8_t destinationIoApicId;
    uint8_t destinationIoApicIntin;
} PACKED;

static struct MpConfigurationTableHeader *header = NULL;
static uint16_t confTableSize = 0;

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

/**
 * @brief Find MP Floating Pointer Structure in one page starting from given address
 * @param *in Memory to start searching from
 * @return Pointer to the beginning of the FPS or NULL on failure
*/
static void *findFloatingPointerStructure(void *in)
{
    for(uint16_t i = 0; i < PAGE_SIZE; i += 16) 
    {
        if(0 == strncmp(MP_FLOATING_POINTER_STRUCTURE_SIGNATURE, 
                        ((const char*)in) + i, 
                        sizeof(MP_FLOATING_POINTER_STRUCTURE_SIGNATURE) - 1))
        {
            return (void*)((uintptr_t)(in) + i);
        }
    }
    return NULL;
}

/**
 * @brief Get Configuration Table Header address
 * @return Configuration table header physical address or 0 on failure
*/
static uintptr_t getConfigurationTableAddress(void)
{
    void *t = MmMapDynamicMemory(0, I686_LOWER_MEMORY_SIZE, 0); //map whole lower memory
    if(NULL == t)
        return 0;

    uintptr_t ebdaOffset = *((uint16_t*)((uintptr_t)t + MP_BDA_MEMORY_SIZE_LOCATION)) * 1024; //get EBDA offset in kilobytes and calculate address in bytes

    uint8_t *fpsr = NULL;

    //floating table pointer structure should lay within the 1st kB of EBDA on a 16-byte boundary
    if((0 == ebdaOffset) || (NULL == (fpsr = findFloatingPointerStructure((void*)((uintptr_t)t + ebdaOffset)))))
    {
        //still might be in last kilobyte of lower memory (639 kB)
        fpsr = findFloatingPointerStructure((void*)((uintptr_t)t + MP_LOWER_MEMORY_LOCATION));

        if(NULL == fpsr) //still no success?
        {
            //find in BIOS memory (64 kiB)
            for(uint16_t i = 0; i < (MP_BIOS_AREA_SPACE_SIZE / PAGE_SIZE); i++)
            {
                if(NULL != (fpsr = findFloatingPointerStructure((void*)((uintptr_t)t + MP_BIOS_AREA_SPACE_LOCATION + (i * PAGE_SIZE)))))
                    break;
            }
            if(NULL == fpsr)
            {
                MmUnmapDynamicMemory(t);
                return 0; //failure
            }
        }
    }

    if(!verifyChecksum(fpsr, MP_FPS_SIZE))
    {
        MmUnmapDynamicMemory(t);
        return 0;
    }

    uint32_t ret = ((uint32_t*)fpsr)[MP_FPS_CONFIGURATION_TABLE_ADDRESS_OFFSET]; //get physical address pointer
    MmUnmapDynamicMemory(t);
    return ret;
}

/**
 * @brief Read configuration table header
 * @return Status code
 * @attention This function leaves the dynamic memory mapped.
*/
static STATUS readConfigurationTableHeader(void)
{
    uintptr_t cth = getConfigurationTableAddress();
    if(0 == cth)
        return NOT_FOUND;
    
    struct MpConfigurationTableHeader *confHeader = MmMapDynamicMemory(cth, PAGE_SIZE, 0);
    if(NULL == confHeader)
        return OUT_OF_RESOURCES;

    //validate signature
    if(0 != strncmp(confHeader->signature, 
                    MP_CONFIGURATION_TABLE_HEADER_SIGNATURE, 
                    sizeof(MP_CONFIGURATION_TABLE_HEADER_SIGNATURE) - 1))
    {
        MmUnmapDynamicMemory(confHeader);
        return NOT_FOUND;
    }

    confTableSize = confHeader->size; //get whole configuration table size
    MmUnmapDynamicMemory(confHeader);
    confHeader = MmMapDynamicMemory(cth, confTableSize, 0);

    if(!verifyChecksum(confHeader, confTableSize))
    {
        MmUnmapDynamicMemory(confHeader);
        return NOT_FOUND;
    }

    header = confHeader;
    return OK;
}

static void readEntries(struct MpConfigurationTableHeader *hdr)
{
    uint16_t entryCount = hdr->entryCount;
    
    void *entry = ++hdr; //get first entry pointer

    while(entryCount--)
    {
        switch(*((uint8_t*)entry))
        {
            case MP_CONFIGURATION_TABLE_ENTRY_CPU:
                entry = ((struct MpProcessorEntry*)entry) + 1;
                break;
            case MP_CONFIGURATION_TABLE_ENTRY_BUS:
                entry = ((struct MpBusEntry*)entry) + 1;
                break;
            case MP_CONFIGURATION_TABLE_ENTRY_IOAPIC:
                entry = ((struct MpIOAPICEntry*)entry) + 1;
                break;
            case MP_CONFIGURATION_TABLE_ENTRY_IO_INTERRUPT:
                struct MpIOInterruptAssignmentEntry* ioirq = (struct MpIOInterruptAssignmentEntry*)entry;
                int pciInt = '?';
                switch(ioirq->sourceBusIrq & 0x3)
                {
                    case 0:
                        pciInt = 'A';
                        break;
                    case 1:
                        pciInt = 'B';
                        break;
                    case 2:
                        pciInt = 'C';
                        break;
                    case 3:
                        pciInt = 'D';
                        break;
                }
                //LOG(SYSLOG_INFO, "Device %u @ bus %u INT%c is mapped to IRQ %u", (unsigned int)(ioirq->sourceBusIrq >> 2), (unsigned int)ioirq->sourceBusId, pciInt, (unsigned int)ioirq->destinationIoApicIntin);
                entry = ioirq + 1;
                break;
            case MP_CONFIGURATION_TABLE_ENTRY_LOCAL_INTERRUPT:
                volatile struct MpLocalInterruptAssignmentEntry* lirq =  (struct MpLocalInterruptAssignmentEntry*)entry;
                entry = ((struct MpLocalInterruptAssignmentEntry*)entry) + 1;
                
                break;
            default:
                break;
        }
    }
}

STATUS AcpiMpAnalyze(void)
{   
    STATUS ret = OK;
    if(OK != (ret = readConfigurationTableHeader()))
        return ret;

    readEntries(header);

    return OK;
}