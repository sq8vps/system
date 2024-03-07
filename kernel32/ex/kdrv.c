#include "kdrv.h"
#include "exec.h"
#include "elf.h"
#include "mm/heap.h"
#include "common.h"
#include "io/fs/fs.h"
#include "ke/core/mutex.h"
#include "mm/mm.h"
#include "ksym.h"
#include <stdbool.h>

#define KDRV_MAX_DRIVER_COUNT 200

//list of loaded kernel drivers
// static struct ExDriverObject *kernelDriverList[KDRV_MAX_DRIVER_COUNT];
// //next sequential driver index (starting from 1)
// static uint32_t kernelDriverNextSeqIndex = 1;

static struct ExDriverObject *kernelDriverListHead = NULL;
static KeMutex kernelDriverListMutex = KeMutexInitializer;

STATUS ExLoadKernelDriversForDevice(const char *deviceId, struct ExDriverObjectList **drivers, uint16_t *driverCount)
{
    if(0 == CmStrcmp("ACPI", deviceId))
    {
        *drivers = MmAllocateKernelHeap(sizeof(struct ExDriverObjectList));
        (*drivers)->next = NULL;
        STATUS ret = ExLoadKernelDriver("/initrd/acpi.drv", &((*drivers)->this));
        if(OK != ret)
            return ret;
        
        ret = (*drivers)->this->init((*drivers)->this);

        if(OK == ret)
            *driverCount = 1;
        
        return ret;
    }
    else if((0 == CmStrcmp("ACPI/PNP0A03", deviceId)) || (0 == CmStrcmp("ACPI/PNP0A08", deviceId)))
    {
        *drivers = MmAllocateKernelHeap(sizeof(struct ExDriverObjectList));
        (*drivers)->next = NULL;
        STATUS ret = ExLoadKernelDriver("/initrd/pci.drv", &((*drivers)->this));
        if(OK != ret)
            return ret;
        
        ret = (*drivers)->this->init((*drivers)->this);

        if(OK == ret)
            *driverCount = 1;
        
        return ret;
    }
    else if((0 == CmStrcmp("PCI/8086/7010", deviceId))
            || (0 == CmStrcmp("PCI/8086/7111", deviceId))
    )
    {
        *drivers = MmAllocateKernelHeap(sizeof(struct ExDriverObjectList));
        (*drivers)->next = NULL;
        STATUS ret = ExLoadKernelDriver("/initrd/ide.drv", &((*drivers)->this));
        if(OK != ret)
            return ret;
        
        ret = (*drivers)->this->init((*drivers)->this);

        if(OK == ret)
            *driverCount = 1;
        
        return ret;
    }
    else if(0 == CmStrncmp("DISK", deviceId, 4))
    {
        *drivers = MmAllocateKernelHeap(sizeof(struct ExDriverObjectList));
        // (*drivers)->next = MmAllocateKernelHeap(sizeof(struct ExDriverObjectList));
        (*drivers)->next = NULL;
        STATUS ret = ExLoadKernelDriver("/initrd/disk.drv", &((*drivers)->this));
        if(OK != ret)
            return ret;
        // ret = ExLoadKernelDriver("/initrd/partmgr.drv", &((*drivers)->next->this));
        // if(OK != ret)
        //     return ret;

        ret = (*drivers)->this->init((*drivers)->this);
        // ret = (*drivers)->next->this->init((*drivers)->next->this);

        if(OK == ret)
            *driverCount = 1;
        
        return ret; 
    }
    
    return IO_FILE_NOT_FOUND;
}

//#define KDRV_INITIAL_INDEX 0
// KDRV_INDEX_T nextDriverIndex = KDRV_INITIAL_INDEX;

// struct KDrv_DriverListEntry
// {
//     KDRV_INDEX_T index; //driver index
//     DDK_KDrvMetadata_t *meta; //driver metadata
//     void (*entry)(KDRV_INDEX_T index); //driver entry point (only for initialization)
//     uintptr_t *vaddr; //driver virtual address
//     uintptr_t size; //driver memory size (heap not included)
//     uint8_t started : 1; //is driver started?

//     DDK_KDrvGeneralCallbacks_t *generalCallbacks; //general driver callbacks
//     KDrv_Callbacks_t *specializedCallbacks; //specialized driver callbacks
    
//     struct KDrv_DriverListEntry *next; //next driver in linked list
// };

// struct KDrv_DriverListEntry *kernelModeDriverList = NULL;

// static void initKernelModeDriverEntry(struct KDrv_DriverListEntry *e)
// {
//     e->entry = NULL;
//     e->vaddr = NULL;
//     e->size = 0;
//     e->started = 0;
//     e->meta = NULL;
//     e->index = 0;
//     e->generalCallbacks = NULL;
// }

// static struct KDrv_DriverListEntry* getLastKernelModeDriverEntry(void)
// {
//     struct KDrv_DriverListEntry *t = kernelModeDriverList;
//     if(NULL == t)
//         return NULL;

//     while(NULL != t->next)
//     {
//         t++;
//     }

//     return t;
// }

// static struct KDrv_DriverListEntry* getKernelModeDriverEntry(const KDRV_INDEX_T index)
// {
//     struct KDrv_DriverListEntry *t = kernelModeDriverList;
//     if(NULL == t)
//         return NULL;

//     while(index != t->index)
//     {
//         if(NULL == t->next)
//             return NULL;
//         t++;
//     }

//     return t;
// }

// static STATUS pushDriverMetadata(const struct KDrv_DriverListEntry *e)
// {
//     struct KDrv_DriverListEntry *m = MmAllocateKernelHeap(sizeof(struct KDrv_DriverListEntry)); //allocate memory for entry
//     if(NULL == m)
//         return EXEC_MALLOC_FAILED;
    
//     CmMemcpy(m, e, sizeof(*m)); //copy entry

//     m->next = NULL; //just to be sure

//     if(NULL == kernelModeDriverList) //initialize head if not initialized
//     {
//         kernelModeDriverList = m;
//     }
//     else //head initialized, update last entry
//     {
//         getLastKernelModeDriverEntry()->next = m;
//     }
    
//     return OK;
// }

STATUS ExLoadKernelDriver(char *path, struct ExDriverObject **driverObject)
{
    if(!IoCheckIfFileExists(path))
	{
        return IO_FILE_NOT_FOUND;
	}

    uint64_t imageSize, freeSize;
    uintptr_t bssSize;
	STATUS ret = OK;
    if(OK != (ret = IoGetFileSize(path, &imageSize)))
    {
		return ret;
	}

    if(OK != (ret = ExGetExecutableRequiredBssSize(path, &bssSize)))
    {
		return ret;
	}

    struct ExDriverObject *object = MmAllocateKernelHeap(sizeof(struct ExDriverObject));
    if(NULL == object)
    {
        return OUT_OF_RESOURCES;
    }

    object->size = imageSize + bssSize;

    KeAcquireMutex(&kernelDriverListMutex);

    struct ExDriverObject *t = kernelDriverListHead;

//TODO: reuse!
    if(NULL == t)
    {
        object->address = MM_DRIVERS_START_ADDRESS;
        freeSize = MM_DRIVERS_MAX_SIZE;
        object->next = NULL;
        object->previous = NULL;
        kernelDriverListHead = object;
    }
    else
    {
        while(t->next)
        {
            t = t->next;
        }
        object->previous = t;
        object->next = NULL;
        t->next = object;
        object->address = ALIGN_UP((t->address + t->size), MM_PAGE_SIZE);
        freeSize = MM_DRIVERS_MAX_SIZE - object->address;
    }

    LOG("Driver %s loaded at 0x%lX\n", path, object->address);

    if(imageSize > freeSize)
    {
        ret = OUT_OF_RESOURCES;
        goto loadKernelDriverFailure;
    }

    IoFileHandle *f = NULL;
    if((OK != (ret = IoOpenKernelFile(path, IO_FILE_READ | IO_FILE_BINARY, 0, &f)) || (NULL == f)))
    {
        goto loadKernelDriverFailure;
	}

    if(OK != (ret = MmAllocateMemory(object->address, imageSize + bssSize, MM_PAGE_FLAG_WRITABLE)))
    {
        IoCloseKernelFile(f);
        goto loadKernelDriverFailure;
    }

	uint64_t actualSize = 0;
	if((OK != (ret = IoReadKernelFile(f, (void*)object->address, imageSize, 0, &actualSize))) || (actualSize != imageSize))
	{
		IoCloseKernelFile(f);
		MmFreeMemory(object->address, object->size);
		if(OK == ret)
			ret = IO_READ_INCOMPLETE;
		goto loadKernelDriverFailure;
	}

	IoCloseKernelFile(f);

	struct Elf32_Ehdr *elfHeader = (struct Elf32_Ehdr*)object->address;
	if(OK != (ret = ExVerifyElf32Header(elfHeader)))
	{
		MmFreeMemory(object->address, object->size);
		goto loadKernelDriverFailure;
	}

	if(ET_REL != elfHeader->e_type)
    {
        MmFreeMemory(object->address, object->size);
        ret = EXEC_ELF_BAD_FORMAT;
        goto loadKernelDriverFailure;
    }


    if(OK != (ret = ExPrepareExecutableBss((void*)object->address, (void*)(object->address + (uintptr_t)imageSize))))
    {
		MmFreeMemory(object->address, object->size);
		goto loadKernelDriverFailure;
	}

    if(OK != (ret = ExPerformElf32Relocation(elfHeader, ExGetKernelSymbol)))
    {
        MmFreeMemory(object->address, object->size);
		goto loadKernelDriverFailure;
    }

    uintptr_t entry;
    if(OK != (ret = ExGetElf32SymbolValueByName(elfHeader, STRINGIFY(DRIVER_ENTRY), &entry)))
    {
        MmFreeMemory(object->address, object->size);
		goto loadKernelDriverFailure;
    }

    if(OK != (ret = ((DRIVER_ENTRY_T*)entry)(object)))
    {
        MmFreeMemory(object->address, object->size);
		goto loadKernelDriverFailure;
    }

    *driverObject = object;
    
    KeReleaseMutex(&kernelDriverListMutex);

    return OK;

loadKernelDriverFailure:
    
    if(NULL != t)
        t->next = NULL;
    if(NULL == object->previous)
        kernelDriverListHead = NULL;
    else
        object->previous->next = object->next;
    
    KeReleaseMutex(&kernelDriverListMutex);
    
    MmFreeKernelHeap(object);
    return ret;
}

struct ExDriverObject *ExFindDriverByAddress(uintptr_t *address)
{
    if(NULL == kernelDriverListHead)
        return NULL;

    KeAcquireMutex(&kernelDriverListMutex);
    struct ExDriverObject *t = kernelDriverListHead;
    
    while(NULL != t)
    {
        if((*address >= t->address) && (*address < (t->address + t->size)))
            break;
        if(NULL == t->next)
        {
            KeReleaseMutex(&kernelDriverListMutex);
            return NULL;
        }
        t = t->next;
    }
    *address = t->address;
    KeReleaseMutex(&kernelDriverListMutex);
    return t;
}

char* ExMakeDeviceId(uint8_t count, ...)
{
    if(0 == count)
        return NULL;
    
    va_list args;
    va_start(args, count);
    uint32_t size = 0;
    for(uint8_t i = 0; i < count; i++)
        size += (CmStrlen(va_arg(args, char*)) + 1);

    char *r = MmAllocateKernelHeap(size);
    if(NULL == r)
        return r;
    r[0] = '\0';

    va_start(args, count);
    for(uint8_t i = 0; i < count; i++)
    {
        CmStrcat(r, va_arg(args, char*));
        if(i != (count - 1))
            CmStrcat(r, "/");
    }
 
    va_end(args);

    for(uint32_t i = 0; i < CmStrlen(r); i++)
    {
        if(' ' == r[i])
            r[i] = '_';
    }

    return r;
}