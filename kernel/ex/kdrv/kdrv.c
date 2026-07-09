#include "kdrv.h"
#include "ex/exec.h"
#include "ex/elf.h"
#include "mm/heap.h"
#include "io/log/syslog.h"
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
#include "hal/mm.h"
#include "rtl/stdlib.h"
#include "rtl/string.h"
#include "io/fs/vfs.h"

uint32_t ExAssignDriverId(void);
void ExFreeDriverId(uint32_t id);

static struct
{
    struct ExDriverObject *list;
    KeMutex mutex;
    char *databasePath;

} ExKernelDriverState = {.list = NULL, .mutex = KeMutexInitializer};

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
        ObDestroyObject(object);
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
        ObDestroyObject(object);
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
        ObDestroyObject(nextObject);
    }

    if(NULL == object->previous)
        ExKernelDriverState.list = object;
    
    KeReleaseMutex(&(ExKernelDriverState.mutex));
}

static STATUS ExLoadKernelDriverImage(const char *path, struct ExDriverObject **driverObject)
{
    STATUS status = OK;
    struct ExDriverObject *object = NULL;
    uint64_t imageSize = 0, freeSize = 0, requiredSize = 0;
    size_t bssSize = 0;

    KeAcquireMutex(&ExKernelDriverState.mutex);
    struct ExDriverObject *drv = ExKernelDriverState.list;
    const char *imageName = RtlGetFileName(path);
    while(NULL != drv)
    {
        if(!RtlStrcmp(drv->imageName, imageName))
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
        return NOT_FOUND;

    status = IoGetFileSize(path, &imageSize);
    if(OK != status)
		return status;

    status = ExGetExecutableRequiredBssSize(path, &bssSize);
    if(OK != status)
		return status;

    requiredSize = ALIGN_UP(imageSize + bssSize, PAGE_SIZE);

    KeAcquireMutex(&(ExKernelDriverState.mutex));

    struct ExDriverObject *t = ExKernelDriverState.list;

    if(NULL != t)
    {
        struct ExDriverObject *bestFit = NULL;
        size_t bestFitSize = UINTPTR_MAX;
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
            
            size_t remaining = freeSize - requiredSize;
            if(remaining >= PAGE_SIZE)
            {
                struct ExDriverObject *nextBlock = NULL;
                nextBlock = ObCreateKernelObject(OB_DRIVER);
                if(NULL != nextBlock)
                {
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
        object = ObCreateKernelObject(OB_DRIVER);
        if(NULL == object)
        {
            KeReleaseMutex(&(ExKernelDriverState.mutex));
            return OUT_OF_RESOURCES;
        }

        object->free = false;
        object->size = requiredSize;
        object->next = NULL;
        if(NULL == t)
        {
            object->address = HalGetDriverSpaceBase();
            freeSize = HalGetDriverSpaceSize();
            object->previous = NULL;
            ExKernelDriverState.list = object;
        }
        else
        {
            object->address = t->address + t->size;
            freeSize = (HalGetDriverSpaceBase() + HalGetDriverSpaceSize()) - object->address;
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

    if(requiredSize > freeSize)
    {
        object->free = true;
        status = OUT_OF_RESOURCES;
        goto LoadKernelDriverFailure;
    }

    int f = -1;
    status = IoOpenFile(path, IO_FILE_READ, 0, &f);
    if(OK != status)
    {
        object->free = true;
        goto LoadKernelDriverFailure;
	}

    status = MmAllocateMemory(object->address, object->size, MM_FLAG_WRITABLE);
    if(OK != status)
    {
        object->free = true;
        IoCloseFile(f);
        goto LoadKernelDriverFailure;
    }

	size_t actualSize = 0;
	status = IoReadFileSync(f, (void*)object->address, imageSize, 0, &actualSize);
    if((OK != status) || (actualSize != imageSize))
	{
		IoCloseFile(f);
		MmFreeMemory(object->address, object->size);
		if(OK == status)
			status = OPERATION_INCOMPLETE;
		goto LoadKernelDriverFailure;
	}

	IoCloseFile(f);

	struct Elf32_Ehdr *elfHeader = (struct Elf32_Ehdr*)object->address;
	status = ExVerifyElf32Header(elfHeader);
    if(OK != status)
	{
		goto LoadKernelDriverFailure;
	}

	if(ET_REL != elfHeader->e_type)
    {
        status = BAD_TYPE;
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

    status = ExGetElf32SymbolValueByName(elfHeader, STRINGIFY(DRIVER_ENTRY), (uintptr_t*)&(object->entry));
    if(OK != status)
    {
		goto LoadKernelDriverFailure;
    }

    const char *c = RtlGetFileName(path);
    object->imageName = MmAllocateKernelHeap(RtlStrlen(c) + 1);
    if(NULL == object->imageName)
    {
        status = OUT_OF_RESOURCES;
        goto LoadKernelDriverFailure;
    }

    RtlStrcpy(object->imageName, c);

    *driverObject = object;
    
    KeReleaseMutex(&(ExKernelDriverState.mutex));

#ifdef DEBUG
    LOG(SYSLOG_INFO, "Driver %s with ID %lu loaded at 0x%lX", path, object->id, object->address);
#else
    LOG(SYSLOG_INFO, "Driver %s loaded", path);
#endif

    return OK;

LoadKernelDriverFailure:
    

    LOG(SYSLOG_INFO, "Driver %s loading failed, error %d", path, status);

    KeReleaseMutex(&(ExKernelDriverState.mutex));
    
    ExRemoveKernelDriverObject(object);
    
    return status;
}

static STATUS ExLoadKernelDrivers(const char *name, bool fs, const char *deviceId, char * const * compatibleIds, struct IoDeviceObject *disk,
    struct ExDriverObjectList **drivers, size_t *driverCount)
{
    STATUS status;
    struct ExDbHandle *configDb = NULL, *deviceIdDb = NULL, *fsDriverDb = NULL, *driverDb = NULL;
    char *dbSearchPath = NULL, *imageSearchPath = NULL;
    char *dbPath = NULL, *imagePath = NULL;
    struct ExDriverObject *drv = NULL;
    char *t = NULL;
    
    size_t maxNameLength = IoVfsGetMaxFileNameLength();

    if(NULL != driverCount)
        *driverCount = 0;

    dbPath = MmAllocateKernelHeap(maxNameLength);
    if(NULL == dbPath)
    {
        status = OUT_OF_RESOURCES;
        goto ExLoadKernelDriversForExit;
    }

    imagePath = MmAllocateKernelHeap(maxNameLength);
    if(NULL == imagePath)
    {
        status = OUT_OF_RESOURCES;
        goto ExLoadKernelDriversForExit;
    }

    KeAcquireMutex(&ExKernelDriverState.mutex);
    status = ExDbOpen(ExKernelDriverState.databasePath, &configDb);
    KeReleaseMutex(&ExKernelDriverState.mutex);
    if(OK != status)
        goto ExLoadKernelDriversForExit;

    if(NULL == name)
    {
        if(!fs)
        {
            char *path = NULL;
            status = ExDbGetNextString(configDb, "DeviceIdDatabase", &path);
            if(OK != status)
                goto ExLoadKernelDriversForExit;

            status = ExDbOpen(path, &deviceIdDb);
            if(OK != status)
                goto ExLoadKernelDriversForExit;
        }
        else //if(fs)
        {
            char *path = NULL;
            status = ExDbGetNextString(configDb, "FsDriverDatabase", &path);
            if(OK != status)
                goto ExLoadKernelDriversForExit;

            status = ExDbOpen(path, &fsDriverDb);
            if(OK != status)
                goto ExLoadKernelDriversForExit;
        }
    }

    status = ExDbGetNextString(configDb, "DriverDatabasePath", &dbSearchPath);
    if(OK != status)
        goto ExLoadKernelDriversForExit;

    RtlStrncpy(dbPath, dbSearchPath, maxNameLength);
    char *dbFileName = dbPath + RtlStrlen(dbPath);

    status = ExDbGetNextString(configDb, "DriverImagePath", &imageSearchPath);
    if(OK != status)
        goto ExLoadKernelDriversForExit;

    RtlStrncpy(imagePath, imageSearchPath, maxNameLength);
    char *imageFileName = imagePath + RtlStrlen(imagePath);


    if((NULL != name) || !fs)
    {

        if(NULL == name)
        {
            if(OK != ExDbGetNextString(deviceIdDb, deviceId, &t))
            {
                if(NULL == compatibleIds)
                {
                    status = NOT_FOUND;
                    goto ExLoadKernelDriversForExit;
                }

                size_t i = 0;
                while(NULL != compatibleIds[i])
                {
                    //TODO: include closest-match search - maybe by the count of ampersands
                    ExDbRewind(deviceIdDb);
                    if(OK == ExDbGetNextString(deviceIdDb, compatibleIds[i], &t))
                        break;
                    else
                        t = NULL;
                    ++i;
                }
            }

            if(NULL == t)
            {
                status = NOT_FOUND;
                goto ExLoadKernelDriversForExit;
            }
        }

        RtlStrncpy(dbFileName, (NULL != name) ? name : t, maxNameLength - (dbFileName - dbPath));
        status = ExDbOpen(dbPath, &driverDb);
        if(OK != status)
            goto ExLoadKernelDriversForExit;

        if(NULL == name)
        {
            bool deviceIdCheckOk = false;
            bool b = false;
            if((OK != ExDbGetNextBool(driverDb, "DeviceDriver", &b)) || (false == b))
            {
                status = BAD_TYPE;
                goto ExLoadKernelDriversForExit;
            }

            while(1) //device id loop
            {
                status = ExDbGetNextString(driverDb, "DeviceId", &t);
                if(OK != status)
                    goto ExLoadKernelDriversForExit;

                if(!RtlStrcmp(t, deviceId))
                {
                    deviceIdCheckOk = true;
                    break;
                }

                if(NULL != compatibleIds)
                {
                    uint32_t i = 0;
                    while(NULL != compatibleIds[i])
                    {
                        if(!RtlStrcmp(compatibleIds[i], t))
                        {
                            deviceIdCheckOk = true;
                            break;
                        }
                        ++i;
                    }
                    if(deviceIdCheckOk)
                        break;
                }
            }

            if(!deviceIdCheckOk)
            {
                status = NOT_FOUND;
                goto ExLoadKernelDriversForExit;
            }
        }
    }
    else //FS driver
    {
ExLoadKernelDriversFsLoop:
        char *t = NULL;
        status = ExDbGetNextString(fsDriverDb, "FsDriverName", &t);
        if(OK != status)
            goto ExLoadKernelDriversForExit;
            
        RtlStrncpy(dbFileName, t, maxNameLength - (dbFileName - dbPath));
        status = ExDbOpen(dbPath, &driverDb);
        if(OK != status)
            goto ExLoadKernelDriversFsLoop;

        bool b = false;
        if((OK != ExDbGetNextBool(driverDb, "FsDriver", &b)) || (false == b))
        {
            ExDbClose(driverDb);
            driverDb = NULL;
            goto ExLoadKernelDriversFsLoop;
        }
    }

    status = ExDbGetNextString(driverDb, "ImageName", &t);
    if(OK != status)
    {
        if(fs)
        {
            ExDbClose(driverDb);
            driverDb = NULL;
            goto ExLoadKernelDriversFsLoop;
        }
        else
        {
            goto ExLoadKernelDriversForExit;
        }
    }
        
    RtlStrncpy(imageFileName, t, maxNameLength - (imageFileName - imagePath));

    status = ExLoadKernelDriverImage(imagePath, &drv);
    if(OK != status)
    {
        if(fs)
        {
            ExDbClose(driverDb);
            driverDb = NULL;
            goto ExLoadKernelDriversFsLoop;
        }
        else
        {
            goto ExLoadKernelDriversForExit;
        }
    }

    if(!(drv->flags & EX_DRIVER_OBJECT_FLAG_LOADED))
    {
        status = drv->entry(drv, name);
        if(OK != status)
        {
            if(fs)
            {
                ExDbClose(driverDb);
                driverDb = NULL;
                goto ExLoadKernelDriversFsLoop;
            }
            else
            {
                goto ExLoadKernelDriversForExit;
            }
        }
        else
            drv->flags |= EX_DRIVER_OBJECT_FLAG_LOADED;
    }
    
    if(fs && (NULL == name))
    {
        if((NULL == drv->verifyFs) 
            || (OK != drv->verifyFs(drv, disk)))
        {
            ExDbClose(driverDb);
            driverDb = NULL;
            goto ExLoadKernelDriversFsLoop;
        }
    }

    //TODO: implement multiple drivers
    struct ExDriverObjectList *d = NULL;
    if(NULL != drivers)
    {
        d = MmAllocateKernelHeap(sizeof(*d));
        if(NULL == d)
        {
            status = OUT_OF_RESOURCES;
            goto ExLoadKernelDriversForExit;
        }

        d->next = NULL;
        d->thisDriver = drv;
        d->isMain = true;

        *drivers = d;
    }

    if(NULL != driverCount)
        *driverCount = 1;
    
ExLoadKernelDriversForExit:
    MmFreeKernelHeap(dbPath);
    MmFreeKernelHeap(imagePath);

    ExDbClose(deviceIdDb);
    ExDbClose(fsDriverDb);
    ExDbClose(driverDb);
    ExDbClose(configDb);

    if(OK == status)
        return OK;

    if(NULL != drivers)
    {
        while(NULL != *drivers)
        {
            d = (*drivers)->next;
            MmFreeKernelHeap(*drivers);
            *drivers = d;
        }
    }

    return status;
}

STATUS ExLoadKernelDriversForDevice(const char *deviceId, char * const * compatibleIds, struct ExDriverObjectList **drivers, size_t *driverCount)
{
    return ExLoadKernelDrivers(NULL, false, deviceId, compatibleIds, NULL, drivers, driverCount);
}

STATUS ExLoadKernelDriversForFilesystem(struct IoVolumeNode *volume, struct ExDriverObjectList **drivers, size_t *driverCount)
{
    return ExLoadKernelDrivers(NULL, true, NULL, NULL, volume->pdo, drivers, driverCount);
}

STATUS ExLoadKernelDriversByName(const char *name, struct ExDriverObjectList **drivers, size_t *driverCount)
{
    return ExLoadKernelDrivers(name, false, NULL, NULL, NULL, drivers, driverCount);
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
    status = ExDbGetNextString(h, "DriverDatabase", &t);
    if(OK != status)
        FAIL_BOOT("Unable to locate initial driver database\n");
    
    ExKernelDriverState.databasePath = MmAllocateKernelHeap(RtlStrlen(t) + 1);
    if(NULL == ExKernelDriverState.databasePath)
        FAIL_BOOT("memory allocation failed");
    
    RtlStrcpy(ExKernelDriverState.databasePath, t);

    ExDbClose(h);

    if(!IoCheckIfFileExists(ExKernelDriverState.databasePath))
        FAIL_BOOT("Missing initial driver database\n");
    
    return OK;
}

STATUS ExUpdateDriverDatabasePath(void)
{
    struct ExDbHandle *h = NULL;
    STATUS status;
    
    status = ExDbOpen(CONFIG_DATABASE, &h);
    if(OK != status)
    {
        IoWriteSyslog(&IoKernelLog, SYSLOG_ERROR, 
            "Unable to open main system configuration database: %s\n", CONFIG_DATABASE);
        return status;
    }
    
    char *t = NULL;
    status = ExDbGetNextString(h, "DriverDatabase", &t);
    ExDbClose(h);
    if(OK != status)
    {
        IoWriteSyslog(&IoKernelLog, SYSLOG_ERROR, 
            "Unable to get driver database\n");
        return status;
    }
    
    KeAcquireMutex(&(ExKernelDriverState.mutex));
    ExKernelDriverState.databasePath = MmAllocateKernelHeap(RtlStrlen(t) + 1);
    if(NULL == ExKernelDriverState.databasePath)
    {
        KeReleaseMutex(&(ExKernelDriverState.mutex));
        IoWriteSyslog(&IoKernelLog, SYSLOG_ERROR, 
            "Unable to allocate memory\n");
        return status;
    }
    
    RtlStrcpy(ExKernelDriverState.databasePath, t);
    KeReleaseMutex(&(ExKernelDriverState.mutex));
    
    return OK;    
}