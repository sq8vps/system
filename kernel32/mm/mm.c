#include "mm.h"
#include "palloc.h"
#include "slab.h"

static void *MmMemoryDescriptorSlabHandle = NULL;
#define MM_MEMORY_DESCRIPTOR_CACHE_CHUNKS_PER_SLAB 128

STATUS MmInitializeMemoryDescriptorAllocator(void)
{
    MmMemoryDescriptorSlabHandle = MmSlabCreate(sizeof(struct MmMemoryDescriptor), MM_MEMORY_DESCRIPTOR_CACHE_CHUNKS_PER_SLAB);
    if(NULL == MmMemoryDescriptorSlabHandle)
        return OUT_OF_RESOURCES;
    
    return OK;
}

struct MmMemoryDescriptor* MmAllocateMemoryDescriptor(void)
{
    return MmSlabAllocate(MmMemoryDescriptorSlabHandle);
}

void MmFreeMemoryDescriptor(struct MmMemoryDescriptor *descriptor)
{
    if(NULL == descriptor)
        return;
    MmSlabFree(MmMemoryDescriptorSlabHandle, descriptor);
}

struct MmMemoryDescriptor* MmBuildMemoryDescriptorList(void *memory, uintptr_t size)
{
    if(0 == size)
        return NULL;
    struct MmMemoryDescriptor *head = MmAllocateMemoryDescriptor();
    if(head == NULL)
        return NULL;
    
    struct MmMemoryDescriptor *d = head;
    d->size = 0;
    d->physical = 0;
    d->mapped = memory;
    d->next = NULL;
    
    while(size)
    {
        uintptr_t physical;
        uintptr_t bytesToBoundary;
        if(OK != MmGetPhysicalPageAddress((uintptr_t)memory, &physical))
            goto _MmBuildMemoryDescriptorListFailure;

        if(physical & (MM_PAGE_SIZE - 1))
            bytesToBoundary = ALIGN_UP(physical, MM_PAGE_SIZE) - physical;
        else
            bytesToBoundary = MM_PAGE_SIZE;
        if(bytesToBoundary > size)
            bytesToBoundary = size;

        if(0 == d->size) //"fresh" block
        {
            d->physical = physical;
            d->size = bytesToBoundary;  
        }
        else if(((uintptr_t)d->physical + d->size) == physical) //adjacent physical address
        {
            d->size += bytesToBoundary;
        }
        else //not a fresh block and physical address is not adjacent
        {
            d->next = MmAllocateMemoryDescriptor();
            if(NULL == d->next)
                goto _MmBuildMemoryDescriptorListFailure;
            
            d = d->next;
            d->physical = physical;
            d->mapped = memory;
            d->size = bytesToBoundary;
            d->next = NULL;
        }

        size -= bytesToBoundary;
        memory = (void*)((uintptr_t)memory + bytesToBoundary);
    }
    return head;

_MmBuildMemoryDescriptorListFailure:
    MmFreeMemoryDescriptorList(head);
    return NULL;
}

void MmFreeMemoryDescriptorList(struct MmMemoryDescriptor *list)
{
    struct MmMemoryDescriptor *t;
    while(NULL != list)
    {
        t = list->next;
        MmFreeMemoryDescriptor(list);
        list = t;
    }
}

STATUS MmAllocateMemory(uintptr_t address, uintptr_t size, MmPagingFlags_t flags)
{
    STATUS ret = OK;

    uintptr_t initialAddress = address;
    while(size)
    {
        if(address & (MM_PAGE_SIZE - 1)) //check if memory is aligned
            return MM_UNALIGNED_MEMORY;
        
        uintptr_t pAddress = 0;
        uintptr_t allocated = MmAllocatePhysicalMemory(size, &pAddress);
        if(0 == allocated) //no memory was allocated - this is an error condition
        {
            ret = MM_NO_MEMORY;
            goto mmAllocateKernelMemoryFailed;
        }

        if(OK != (ret = MmMapMemoryEx(address, pAddress, allocated, flags)))
        {
            //mapping failed
            MmFreePhysicalMemory(pAddress, MM_PAGE_SIZE); //free physical page that wasn't mapped
            goto mmAllocateKernelMemoryFailed;
        }
        address += allocated;
        size -= allocated;
    }

    return OK;

    mmAllocateKernelMemoryFailed:
    address -= MM_PAGE_SIZE;
    uintptr_t pAddress = 0;
    //unmap and free previously mapped and allocated pages
    while(address != initialAddress)
    {
        MmGetPhysicalPageAddress(address, &pAddress);
        MmFreePhysicalMemory(pAddress, MM_PAGE_SIZE);
        MmUnmapMemory(address);
        address -= MM_PAGE_SIZE;
    }
    return ret;
}

STATUS MmFreeMemory(uintptr_t address, uintptr_t size)
{
    while(size)
    {
        uintptr_t pAddress = 0;
        if(OK == MmGetPhysicalPageAddress(address, &pAddress))
        {
            MmUnmapMemory(address);
            MmFreePhysicalMemory(pAddress, MM_PAGE_SIZE);
        }
        address -= MM_PAGE_SIZE;
        if(size >= MM_PAGE_SIZE)
            size -= MM_PAGE_SIZE;
        else
            size = 0;
    }
    return OK;
}

