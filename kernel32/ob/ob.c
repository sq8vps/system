#include "ob.h"
#include "ke/core/panic.h"

#define OB_HEADER_MAGIC (((uint32_t)'K') | ((uint32_t)'E' << 8) | ((uint32_t)'O' << 16) | ((uint32_t)'B' << 24))


void ObInitializeObjectHeader(void *object)
{
    struct ObObjectHeader *h = object;
    h->magic = OB_HEADER_MAGIC;
    h->lock = (KeSpinlock)KeSpinlockInitializer;
}

PRIO ObLockObject(void *object)
{
    struct ObObjectHeader *h = object;
    if(OB_HEADER_MAGIC != h->magic)
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), OBJECT_LOCK_UNAVAILABLE, (uintptr_t)object, 0, 0, 0);
    return KeAcquireSpinlock(&h->lock);
}

void ObUnlockObject(void *object, PRIO previousPriority)
{
    struct ObObjectHeader *h = object;
    if(OB_HEADER_MAGIC != h->magic)
        KePanicIPEx(KE_GET_CALLER_ADDRESS(0), OBJECT_LOCK_UNAVAILABLE, (uintptr_t)object, 1, 0, 0);
    KeReleaseSpinlock(&h->lock, previousPriority);
}