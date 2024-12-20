#include "ob.h"
#include "ke/core/panic.h"
#include "mm/slab.h"
#include "ke/sched/sched.h"
#include "io/fs/vfs.h"
#include "io/fs/fs.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "ex/kdrv/kdrv.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "io/dev/vol.h"
#include "io/log/syslog.h"

#define OB_HEADER_MAGIC (((uint32_t)'K') | ((uint32_t)'E' << 8) | ((uint32_t)'O' << 16) | ((uint32_t)'B' << 24))
#define OB_CHUNK_COUNT 16

static void *ObCache[OB_TYPE_COUNT] = {[0 ... OB_TYPE_COUNT - 1] = NULL};
static size_t ObSize[OB_TYPE_COUNT] = {
    0,
    sizeof(KeSpinlock),
    sizeof(KeMutex),
    sizeof(KeSemaphore),
    sizeof(KeRwLock),
    sizeof(struct KeProcessControlBlock),
    sizeof(struct KeTaskControlBlock),
    sizeof(struct IoFileHandle),
    sizeof(struct IoVfsNode),
    sizeof(struct ExDriverObject),
    sizeof(struct IoDeviceObject),
    sizeof(struct IoDeviceNode),
    sizeof(struct IoRp),
    sizeof(struct IoVolumeNode),
    sizeof(struct IoSyslogHandle),
};

static inline void *ObCreateObjectWithOwner(enum ObObjectType type, size_t additional, struct KeProcessControlBlock *pcb)
{
    if((OB_UNKNOWN == type) || (type >= OB_TYPE_COUNT))
        return NULL;
    
    struct ObObjectHeader *o = NULL;
    if(0 != additional)
    {
        o = MmAllocateKernelHeapZeroed(additional + ObSize[type]);
        if(NULL == o)
            return NULL;
        o->extended = true;
    }
    else
    {
        o = MmSlabAllocate(ObCache[type]);
        if(NULL == o)
            return NULL;
        RtlMemset(o, 0, ObSize[type]);
        o->extended = false;
    }
    o->magic = OB_HEADER_MAGIC;
    o->type = type;
    o->mutex = (KeMutex)KeMutexInitializer;
    o->owner = pcb;
    return o;
}

void *ObCreateKernelObjectEx(enum ObObjectType type, size_t additional)
{
    return ObCreateObjectWithOwner(type, additional, NULL);
}

void *ObCreateKernelObject(enum ObObjectType type)
{
    return ObCreateObjectWithOwner(type, 0, NULL);
}

void *ObCreateObjectEx(enum ObObjectType type, size_t additional)
{
    return ObCreateObjectWithOwner(type, additional, KeGetCurrentTaskParent());
}

void *ObCreateObject(enum ObObjectType type)
{
    return ObCreateObjectWithOwner(type, 0, KeGetCurrentTaskParent());
}

STATUS ObChangeObjectOwner(void *object, struct KeProcessControlBlock *pcb)
{
    struct ObObjectHeader *h = object;
    if(unlikely(OB_HEADER_MAGIC != h->magic))
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), OBJECT_LOCK_UNAVAILABLE, (uintptr_t)object, 0, 0, 0);
    
    h->owner = pcb;
    return OK;
}

void ObDestroyObject(void *object)
{
    struct ObObjectHeader *h = object;
    if(unlikely(OB_HEADER_MAGIC != h->magic))
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), OBJECT_LOCK_UNAVAILABLE, (uintptr_t)object, 0, 0, 0);   

    if(h->extended)
        MmFreeKernelHeap(h);
    else
        MmSlabFree(ObCache[h->type], h);
}

void ObLockObject(void *object)
{
    struct ObObjectHeader *h = object;
    if(unlikely(OB_HEADER_MAGIC != h->magic))
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), OBJECT_LOCK_UNAVAILABLE, (uintptr_t)object, 0, 0, 0);
    KeAcquireMutex(&(h->mutex));
}

void ObUnlockObject(void *object)
{
    struct ObObjectHeader *h = object;
    if(unlikely(OB_HEADER_MAGIC != h->magic))
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), OBJECT_LOCK_UNAVAILABLE, (uintptr_t)object, 1, 0, 0);
    KeReleaseMutex(&(h->mutex));
}

void ObInitialize(void)
{
    for(uint32_t i = 1; i < OB_TYPE_COUNT; i++)
    {
        ObCache[i] = MmSlabCreate(ALIGN_UP(ObSize[i], 8), OB_CHUNK_COUNT);
        if(NULL == ObCache[i])
            KePanicEx(BOOT_FAILURE, 0, 0, 0, 0);
    }
}