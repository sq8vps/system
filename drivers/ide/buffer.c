#include "device.h"
#include "logging.h"
#include "mm/heap.h"
#include "mm/palloc.h"
#include "mm/dynmap.h"

#define IDE_BUFFER_BLOCK_SIZE 65536

#define IDE_MAX_PRD_ENTRIES 1024
#define IDE_PRD_TABLE_ALIGNMENT 65536

#define IDE_PRD_ENTRY_EOT_FLAG 0x8000

#define IDE_DEFAULT_BUFFER_BLOCK_COUNT 256 //default number of 64 KiB buffer blocks 

// STATUS IdeInitializeBuffers(struct IdeDeviceData *info)
// {
//     for(uint32_t i = 0; i < IDE_DEFAULT_BUFFER_BLOCK_COUNT; i++)
//     {
//         uintptr_t address;
//         if(IDE_BUFFER_BLOCK_SIZE != MmAllocateContiguousPhysicalMemory(IDE_BUFFER_BLOCK_SIZE, &address, IDE_BUFFER_BLOCK_SIZE))
//             return OUT_OF_RESOURCES;
//         struct IoMemoryDescriptor *mem = MmAllocateKernelHeap(sizeof(*mem));
//         if(NULL == mem)
//         {
//             MmFreePhysicalMemory(address, IDE_BUFFER_BLOCK_SIZE);
//             struct IoMemoryDescriptor *t = info->channel[]
//         }
//     }
// }



STATUS IdeClearPrdTable(struct IdePrdTable *t)
{
    if((NULL == t) || (NULL == t->table))
        return NULL_POINTER_GIVEN;
    
    CmMemset(t->table, 0, IDE_MAX_PRD_ENTRIES * sizeof(*(t->table)));
    return OK;
}

STATUS IdeInitializePrdTables(struct IdeDeviceData *info)
{
    if(NULL == info)
        return NULL_POINTER_GIVEN;

    for(uint16_t t = 0; t < 2; t++)
    {
        uintptr_t size;
        if((IDE_MAX_PRD_ENTRIES * sizeof(struct IdePrdEntry))
            > (size = MmAllocateContiguousPhysicalMemory(
                IDE_MAX_PRD_ENTRIES * sizeof(struct IdePrdEntry), 
                &(info->channel[t].prdt.physical), 
                IDE_PRD_TABLE_ALIGNMENT)))
        {
            if(0 != info->channel[t].prdt.physical)
                MmFreePhysicalMemory(info->channel[t].prdt.physical, size);
            LOG(SYSLOG_ERROR, "Failed to allocate memory for PRD table\n");
            return OUT_OF_RESOURCES;
        }
        if(NULL == (info->channel[t].prdt.table = MmMapDynamicMemory(info->channel[t].prdt.physical, size, 0)))
        {
            MmFreePhysicalMemory(info->channel[t].prdt.physical, size);
            LOG(SYSLOG_ERROR, "Failed to map memory for PRD table\n");
            return OUT_OF_RESOURCES;
        }
        IdeClearPrdTable(&(info->channel[t].prdt));
    }

    return OK;
}

uint32_t IdeAddPrdEntry(struct IdePrdTable *table, uint32_t address, uint16_t size)
{
    for(uint32_t i = 0; i < IDE_MAX_PRD_ENTRIES; i++)
    {
        if(!(table->table[i].eot & IDE_PRD_ENTRY_EOT_FLAG))
        {
            if(address > 0)
            {
                table->table[i].address = address;
                table->table[i].size = size;
                table->table[i].eot = IDE_PRD_ENTRY_EOT_FLAG;
                if(i > 0)
                    table->table[i - 1].eot = 0;
            }
            return IDE_MAX_PRD_ENTRIES - i - 1;
        }
    }
    return 0;
}
