#include "kdrv.h"
#include "exec.h"
#include "elf.h"
#include "mm/heap.h"
#include "common.h"
#include "io/fs/fs.h"
#include "ke/core/mutex.h"
#include "mm/mm.h"
#include <stdbool.h>

#define KDRV_MAX_DRIVER_COUNT 200

//list of loaded kernel drivers
static struct ExDriverObject *kernelDriverList[KDRV_MAX_DRIVER_COUNT];
//next sequential driver index (starting from 1)
static uint32_t kernelDriverNextSeqIndex = 1;

static struct ExDriverObject *kernelDriverListHead = NULL;
static KeMutex kernelDriverListMutex = KeMutexInitializer;

STATUS ExLoadKernelDriversForDevice(const char *deviceId, struct ExDriverObject **drivers, uint16_t *driverCount)
{
    return NOT_IMPLEMENTED;
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

STATUS ExLoadKernelDriver(const char *path, struct ExDriverObject **driverObject)
{
    if(!IoCheckIfFileExists(path))
	{
        return IO_FILE_NOT_FOUND;
	}

    uint64_t imageSize, freeSize, bssSize;
	STATUS ret = OK;
    if(OK != (ret = IoGetFileSize(path, &imageSize)))
    {
		return ret;
	}

    uint64_t bssSize = 0;
    if(OK != (ret = ExGetExecutableRequiredBssSize(path, &bssSize)))
    {
		return ret;
	}
    imageSize += bssSize;

    struct ExDriverObject *object = MmAllocateKernelHeap(sizeof(struct ExDriverObject));
    if(NULL == object)
    {
        return OUT_OF_RESOURCES;
    }

    object->size = imageSize;

    KeAcquireMutex(&kernelDriverListMutex);

    struct ExDriverObject *t = kernelDriverListHead;

//TODO: reuse!
    if(NULL == kernelDriverListHead)
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
        freeSize = MM_DRIVERS_MAX_SIZE - (t->address + t->size);
        object->address = t->address + t->size;
    }

    if(imageSize < freeSize)
    {
        ret = OUT_OF_RESOURCES;
        goto loadKernelDriverFailure;
    }

    IoFileHandle *f = NULL;
    if((OK != (ret = IoOpenKernelFile(path, IO_FILE_READ | IO_FILE_BINARY, 0, &f)) || (NULL == f)))
    {
        goto loadKernelDriverFailure;
	}
    
    if(OK != (ret = MmAllocateMemory(object->address, object->size, MM_PAGE_FLAG_WRITABLE)))
    {
        IoCloseKernelFile(f);
        goto loadKernelDriverFailure;
    }

	uint64_t actualSize = 0;
	if((OK != (ret = IoReadKernelFile(f, (void*)object->address, object->size, 0, &actualSize))) || (actualSize != object->size))
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

    if(OK != (ret = ExPrepareExecutableBss((void*)object->address, (void*)(object->address + object->size - bssSize))))
    {
		MmFreeMemory(object->address, object->size);
		goto loadKernelDriverFailure;
	}

loadKernelDriverFailure:
    t->next = NULL;
    if(NULL == object->previous)
        kernelDriverListHead = NULL;
    else
        object->previous->next = object->next;
    
    KeReleaseMutex(&kernelDriverListMutex);
    
    MmFreeKernelHeap(object);
    return ret;
}
