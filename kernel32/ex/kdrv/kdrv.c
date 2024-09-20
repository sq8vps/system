#include "kdrv.h"
#include "ex/exec.h"
#include "ex/elf.h"
#include "mm/heap.h"
#include "common.h"
#include "io/fs/fs.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "ke/core/mutex.h"
#include "mm/mm.h"
#include "ex/ksym.h"
#include <stdbool.h>

uint32_t ExAssignDriverId(void);
void ExFreeDriverId(uint32_t id);

struct
{
    struct ExDriverObject *list;
    KeMutex mutex;
} static ExKernelDriverState = {.list = NULL, .mutex = KeMutexInitializer};

static void ExRemoveKernelDriverObject(struct ExDriverObject *object)
{
    KeAcquireMutex(&(ExKernelDriverState.mutex));
    if(!object->free)
    {
        MmFreeMemory(object->address, object->size);
    }

    if(0 != object->id)
        ExFreeDriverId(object->id);
    
    //last object in the list, deallocate memory and remove completely
    if(NULL == object->next)
    {
        if(NULL != object->previous)
            object->previous->next = NULL;
        
        KeReleaseMutex(&(ExKernelDriverState.mutex));
        MmFreeKernelHeap(object);
        return;
    }
    //there exists a preceding object that is free
    if(NULL != object->previous && (object->previous->free))
    {
        struct ExDriverObject *previousObject = object->previous;
        previousObject->size += object->size;
        if(NULL != object->next)
            object->next->previous = previousObject;
        previousObject->next = object->next;
        MmFreeKernelHeap(object);
        object = previousObject;
    }

    //there exists a succeeding object that is free
    if((NULL != object->next) && (object->next->free))
    {
        struct ExDriverObject *nextObject = object->next;
        object->size += nextObject->size;
        if(NULL != nextObject->next)
            nextObject->next->previous = object;
        object->next = nextObject->next;
        MmFreeKernelHeap(nextObject);
    }

    if(NULL == object->previous)
        ExKernelDriverState.list = object;
    
    KeReleaseMutex(&(ExKernelDriverState.mutex));
}

STATUS ExLoadKernelDriversForDevice(const char *deviceId, struct ExDriverObjectList **drivers, uint16_t *driverCount)
{
    if(0 == CmStrcmp("ACPI", deviceId))
    {
        *drivers = MmAllocateKernelHeap(sizeof(struct ExDriverObjectList));
        (*drivers)->next = NULL;
        STATUS ret = ExLoadKernelDriver("/initrd/drivers/acpi.drv", &((*drivers)->this));
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
        STATUS ret = ExLoadKernelDriver("/initrd/drivers/pci.drv", &((*drivers)->this));
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
        STATUS ret = ExLoadKernelDriver("/initrd/drivers/ide.drv", &((*drivers)->this));
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
        STATUS ret = ExLoadKernelDriver("/initrd/drivers/disk.drv", &((*drivers)->this));
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
    STATUS status = OK;
    struct ExDriverObject *object = NULL;
    uint64_t imageSize = 0, freeSize = 0, requiredSize = 0;
    uintptr_t bssSize = 0;

    if(!IoCheckIfFileExists(path))
        return IO_FILE_NOT_FOUND;

    status = IoGetFileSize(path, &imageSize);
    if(OK != status)
		return status;

    status = ExGetExecutableRequiredBssSize(path, &bssSize);
    if(OK != status)
		return status;

    requiredSize = ALIGN_UP(imageSize + bssSize, MM_PAGE_SIZE);

    KeAcquireMutex(&(ExKernelDriverState.mutex));

    struct ExDriverObject *t = ExKernelDriverState.list;

    if(NULL != t)
    {
        struct ExDriverObject *bestFit = NULL;
        uintptr_t bestFitSize = UINTPTR_MAX;
        while(NULL != t)
        {
            if(t->free)
            {
                if((t->size >= requiredSize) && (t->size < bestFitSize))
                {
                    bestFitSize = t->size;
                    bestFit = t;
                }
            }
            if(NULL != t->next)
                t = t->next;
            else
                break;
        } 
        
        if(NULL != bestFit)
        {
            bestFit->free = false;
            object = bestFit;
            freeSize = bestFit->size;
            
            uintptr_t remaining = freeSize - requiredSize;
            if(remaining >= MM_PAGE_SIZE)
            {
                struct ExDriverObject *nextBlock = NULL;
                nextBlock = MmAllocateKernelHeapZeroed(sizeof(*nextBlock));
                if(NULL != nextBlock)
                {
                    ObInitializeObjectHeader(nextBlock);
                    nextBlock->free = true;
                    nextBlock->size = remaining;
                    bestFit->size = requiredSize;
                    nextBlock->address = bestFit->address + bestFit->size;
                    freeSize = requiredSize;
                    if(NULL != bestFit->next)
                        bestFit->next->previous = nextBlock;
                    nextBlock->next = bestFit->next;
                    nextBlock->previous = bestFit;
                    bestFit->next = nextBlock;
                }
            }
        }
    }

    if(NULL == object)
    {
        object = MmAllocateKernelHeapZeroed(sizeof(*object));
        if(NULL == object)
        {
            KeReleaseMutex(&(ExKernelDriverState.mutex));
            return OUT_OF_RESOURCES;
        }
        ObInitializeObjectHeader(object);

        object->free = false;
        object->size = requiredSize;
        object->next = NULL;
        if(NULL == t)
        {
            object->address = MM_DRIVERS_START_ADDRESS;
            freeSize = MM_DRIVERS_MAX_SIZE;
            object->previous = NULL;
            ExKernelDriverState.list = object;
        }
        else
        {
            object->address = t->address + t->size;
            freeSize = (MM_DRIVERS_START_ADDRESS + MM_DRIVERS_MAX_SIZE) - object->address;
            object->previous = t;
            t->next = object;
        }
    }

    object->id = ExAssignDriverId();
    if(0 == object->id)
    {
        //TODO: implement correct C++ support and get rid of these hacks
        object->id = ExAssignDriverId();
        if(0 == object->id)
        {
            object->free = true;
            status = OUT_OF_RESOURCES;
            goto LoadKernelDriverFailure;
        }
    }

    LOG("Driver %s with ID %lu loaded at 0x%lX\n", path, object->id, object->address);

    if(requiredSize > freeSize)
    {
        object->free = true;
        status = OUT_OF_RESOURCES;
        goto LoadKernelDriverFailure;
    }

    IoFileHandle *f = NULL;
    status = IoOpenKernelFile(path, IO_FILE_READ, 0, &f);
    if(OK != status)
    {
        object->free = true;
        goto LoadKernelDriverFailure;
	}

    status = MmAllocateMemory(object->address, object->size, MM_FLAG_WRITABLE);
    if(OK != status)
    {
        object->free = true;
        IoCloseKernelFile(f);
        goto LoadKernelDriverFailure;
    }

	uint64_t actualSize = 0;
	status = IoReadKernelFileSync(f, (void*)object->address, imageSize, 0, &actualSize);
    if((OK != status) || (actualSize != imageSize))
	{
		IoCloseKernelFile(f);
		MmFreeMemory(object->address, object->size);
		if(OK == status)
			status = IO_READ_INCOMPLETE;
		goto LoadKernelDriverFailure;
	}

	IoCloseKernelFile(f);

	struct Elf32_Ehdr *elfHeader = (struct Elf32_Ehdr*)object->address;
	status = ExVerifyElf32Header(elfHeader);
    if(OK != status)
	{
		goto LoadKernelDriverFailure;
	}

	if(ET_REL != elfHeader->e_type)
    {
        status = EXEC_ELF_BAD_FORMAT;
        goto LoadKernelDriverFailure;
    }


    status = ExPrepareExecutableBss((void*)object->address, (void*)(object->address + (uintptr_t)imageSize));
    if(OK != status)
    {
		goto LoadKernelDriverFailure;
	}

    status = ExPerformElf32Relocation(elfHeader, ExGetKernelSymbol);
    if(OK != status)
    {
		goto LoadKernelDriverFailure;
    }

    uintptr_t entry;
    status = ExGetElf32SymbolValueByName(elfHeader, STRINGIFY(DRIVER_ENTRY), &entry);
    if(OK != status)
    {
		goto LoadKernelDriverFailure;
    }

    status = ((DRIVER_ENTRY_T*)entry)(object);
    if(OK != status)
    {
		goto LoadKernelDriverFailure;
    }

    *driverObject = object;
    
    KeReleaseMutex(&(ExKernelDriverState.mutex));

    return OK;

LoadKernelDriverFailure:
    
    KeReleaseMutex(&(ExKernelDriverState.mutex));
    
    ExRemoveKernelDriverObject(object);
    
    return status;
}

struct ExDriverObject *ExFindDriverByAddress(uintptr_t *address)
{
    struct ExDriverObject *t = ExKernelDriverState.list;
    
    while(NULL != t)
    {
        if((*address >= t->address) && (*address < (t->address + t->size)))
        {
            *address = t->address;
            return t;
        }
        t = t->next;
    }
    return NULL;
}

