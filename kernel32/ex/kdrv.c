#include "kdrv.h"
#include "exec.h"
#include "elf.h"
#include "mm/heap.h"
#include "common.h"
#include "io/fs/fs.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "ke/core/mutex.h"
#include "mm/mm.h"
#include "ksym.h"
#include <stdbool.h>

#define KDRV_MAX_DRIVER_COUNT 200

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

    ObInitializeObjectHeader(object);

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