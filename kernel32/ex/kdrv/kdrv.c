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
#include "ex/db/db.h"
#include "config.h"
#include "ke/core/panic.h"
#include "io/dev/vol.h"

uint32_t ExAssignDriverId(void);
void ExFreeDriverId(uint32_t id);

struct
{
    struct ExDriverObject *list;
    KeMutex mutex;
    char *databasePath;
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
    
    MmFreeKernelHeap(object->imageName);
    
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

static STATUS ExLoadKernelDriverFromFile(const char *path, struct ExDriverObject **driverObject)
{
    STATUS status = OK;
    struct ExDriverObject *object = NULL;
    uint64_t imageSize = 0, freeSize = 0, requiredSize = 0;
    uintptr_t bssSize = 0;

    KeAcquireMutex(&ExKernelDriverState.mutex);
    struct ExDriverObject *drv = ExKernelDriverState.list;
    const char *imageName = CmGetFileName(path);
    while(NULL != drv)
    {
        if(!CmStrcmp(drv->imageName, imageName))
            break;
        drv = drv->next;
    }
    KeReleaseMutex(&ExKernelDriverState.mutex);

    if(NULL != drv)
    {
        *driverObject = drv;
        return OK;
    }

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
        object->free = true;
        status = OUT_OF_RESOURCES;
        goto LoadKernelDriverFailure;
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

    const char *c = CmGetFileName(path);
    object->imageName = MmAllocateKernelHeap(CmStrlen(c) + 1);
    if(NULL == object->imageName)
    {
        status = OUT_OF_RESOURCES;
        goto LoadKernelDriverFailure;
    }

    CmStrcpy(object->imageName, c);

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

static STATUS ExLoadKernelDrivers(bool fs, const char *deviceId, char * const * compatibleIds, struct IoDeviceObject *disk,
    struct ExDriverObjectList **drivers, uint16_t *driverCount)
{
    STATUS status;
    struct ExDbHandle *dbConfig = NULL, *driverConfig = NULL;
    char *dbSearchPath = NULL, *imageSearchPath = NULL;
    char *dbPath = NULL, *imagePath = NULL;
    struct ExDriverObject *drv = NULL;
    
    uint32_t maxNameLength = IoVfsGetMaxFileNameLength();

    *driverCount = 0;

    dbPath = MmAllocateKernelHeap(maxNameLength);
    if(NULL == dbPath)
    {
        status = OUT_OF_RESOURCES;
        goto ExLoadKernelDriversForDeviceFailed;
    }

    imagePath = MmAllocateKernelHeap(maxNameLength);
    if(NULL == imagePath)
    {
        status = OUT_OF_RESOURCES;
        goto ExLoadKernelDriversForDeviceFailed;
    }

    KeAcquireMutex(&ExKernelDriverState.mutex);
    status = ExDbOpen(ExKernelDriverState.databasePath, &dbConfig);
    KeReleaseMutex(&ExKernelDriverState.mutex);

    if(OK != status)
        goto ExLoadKernelDriversForDeviceFailed;
    
    status = ExDbGetNextString(dbConfig, "DatabasePath", &dbSearchPath);
    if(OK != status)
        goto ExLoadKernelDriversForDeviceFailed;

    CmStrncpy(dbPath, dbSearchPath, maxNameLength);
    char *dbFileNamePart = dbPath + CmStrlen(dbPath);

    status = ExDbGetNextString(dbConfig, "ImagePath", &imageSearchPath);
    if(OK != status)
        goto ExLoadKernelDriversForDeviceFailed;

    CmStrncpy(imagePath, imageSearchPath, maxNameLength);
    char *imageFileNamePart = imagePath + CmStrlen(imagePath);

    while(1) //driver database loop
    {
ExLoadKernelDriversLoop:
        char *t = NULL;
        status = ExDbGetNextString(dbConfig, "DriverDatabaseName", &t);
        if(OK != status)
            goto ExLoadKernelDriversForDeviceFailed;
        
        CmStrncpy(dbFileNamePart, t, maxNameLength - (dbFileNamePart - dbPath));

        status = ExDbOpen(dbPath, &driverConfig);
        if(OK != status)
        {
            driverConfig = NULL;
            continue;
        }

        status = ExDbGetNextString(driverConfig, "ImageName", &t);
        if(OK != status)
        {
            ExDbClose(driverConfig);
            continue;
        }
        
        CmStrncpy(imageFileNamePart, t, maxNameLength - (imageFileNamePart - imagePath));

        if(!fs)
        {
            bool b = false;
            if((OK != ExDbGetNextBool(driverConfig, "DeviceDriver", &b)) || (false == b))
            {
                ExDbClose(driverConfig);
                driverConfig = NULL;
                continue;
            }

            while(1) //device id loop
            {
                status = ExDbGetNextString(driverConfig, "DeviceId", &t);
                if(OK != status)
                {
                    ExDbClose(driverConfig);
                    driverConfig = NULL;
                    goto ExLoadKernelDriversLoop;
                }

                if(!CmStrcmp(t, deviceId))
                    //TODO: include best-match search
                    break;

                if(NULL != compatibleIds)
                {
                    uint32_t i = 0;
                    while(NULL != compatibleIds[i])
                    {
                        if(!CmStrcmp(compatibleIds[i], t))
                            goto ExLoadKernelDriversFound;
                        ++i;
                    }
                }
            }
        }
        else //if(fs)
        {
            bool b = false;
            if((OK != ExDbGetNextBool(driverConfig, "FsDriver", &b)) || (false == b))
            {
                ExDbClose(driverConfig);
                driverConfig = NULL;
                continue;
            }

            status = ExDbGetNextString(driverConfig, "ImageName", &t);
            if(OK != status)
            {
                ExDbClose(driverConfig);
                driverConfig = NULL;
                continue;
            }
        }

ExLoadKernelDriversFound:
        status = ExLoadKernelDriverFromFile(imagePath, &drv);
        if(OK != status)
        {
            ExDbClose(driverConfig);
            driverConfig = NULL;
            continue;
        }
        
        if(!(drv->flags & EX_DRIVER_OBJECT_FLAG_INITIALIZED))
        {
            if(NULL != drv->init)
                status = drv->init(drv);
            else
                status = OK;

            if(OK != status)
            {
                ExDbClose(driverConfig);
                driverConfig = NULL;
                continue;
            }
            drv->flags |= EX_DRIVER_OBJECT_FLAG_INITIALIZED;
        }

        if(fs)
        {
            if(NULL != drv->verifyFs)
            {
                if(OK == drv->verifyFs(drv, disk))
                    break;
            }

            ExDbClose(driverConfig);
            driverConfig = NULL;
            continue;
        }
        else //if(!fs)
            break;
    }

    //TODO: implement multiple drivers
    struct ExDriverObjectList *d = MmAllocateKernelHeap(sizeof(*d));
    if(NULL == d)
    {
        status = OUT_OF_RESOURCES;
        goto ExLoadKernelDriversForDeviceFailed;
    }

    d->next = NULL;
    d->this = drv;
    d->isMain = true;

    *drivers = d;
    *driverCount = 1;
    
ExLoadKernelDriversForDeviceFailed:
    MmFreeKernelHeap(dbPath);
    MmFreeKernelHeap(imagePath);

    ExDbClose(driverConfig);
    ExDbClose(dbConfig);

    if(OK == status)
        return OK;

    while(NULL != *drivers)
    {
        d = (*drivers)->next;
        MmFreeKernelHeap(*drivers);
        *drivers = d;
    }

    return status;
}

STATUS ExLoadKernelDriversForDevice(const char *deviceId, char * const * compatibleIds, struct ExDriverObjectList **drivers, uint16_t *driverCount)
{
    return ExLoadKernelDrivers(false, deviceId, compatibleIds, NULL, drivers, driverCount);
}

STATUS ExLoadKernelDriversForFilesystem(struct IoVolumeNode *volume, struct ExDriverObjectList **drivers, uint16_t *driverCount)
{
    return ExLoadKernelDrivers(true, NULL, NULL, volume->pdo, drivers, driverCount);
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

STATUS ExInitializeDriverManager(void)
{
    struct ExDbHandle *h = NULL;
    STATUS status;
    
    status = ExDbOpen(INITIAL_CONFIG_DATABASE, &h);
    if(OK != status)
        FAIL_BOOT("Unable to open initial system configuration database\n");
    
    char *t = NULL;
    status = ExDbGetNextString(h, "DriverDatabasePath", &t);
    if(OK != status)
        FAIL_BOOT("Unable to locate initial driver database\n");
    
    ExKernelDriverState.databasePath = MmAllocateKernelHeap(CmStrlen(t) + 1);
    if(NULL == ExKernelDriverState.databasePath)
        FAIL_BOOT("memory allocation failed");
    
    CmStrcpy(ExKernelDriverState.databasePath, t);

    ExDbClose(h);

    if(!IoCheckIfFileExists(ExKernelDriverState.databasePath))
        FAIL_BOOT("Missing initial driver database\n");
    
    return OK;
}