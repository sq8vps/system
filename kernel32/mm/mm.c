#include "mm.h"
#include "palloc.h"
#include "slab.h"
#include "dynmap.h"
#include "hal/mm.h"
#include "hal/arch.h"
#include "rtl/string.h"

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
        if(OK != HalGetPhysicalAddress((uintptr_t)memory, &physical))
            goto MmBuildMemoryDescriptorListFailure;

        if(physical & (PAGE_SIZE - 1))
            bytesToBoundary = ALIGN_UP(physical, PAGE_SIZE) - physical;
        else
            bytesToBoundary = PAGE_SIZE;
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
                goto MmBuildMemoryDescriptorListFailure;
            
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

MmBuildMemoryDescriptorListFailure:
    MmFreeMemoryDescriptorList(head);
    return NULL;
}

struct MmMemoryDescriptor *MmCloneMemoryDescriptorList(struct MmMemoryDescriptor *list)
{
    if(NULL == list)
        return NULL;

    struct MmMemoryDescriptor *head = MmAllocateMemoryDescriptor(), *current;
    if(NULL == head)
        return NULL;
    
    current = head;
    *current = *list;
    current->next = NULL;
    list = list->next;
    
    while(NULL != list)
    {
        current->next = MmAllocateMemoryDescriptor();
        if(NULL == current->next)
        {
            MmFreeMemoryDescriptorList(head);
            return NULL;
        }
        current = current->next;
        *current = *list;
        list = list->next;
        if(NULL == list)
            current->next = NULL;
    }
    return head;
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

uint64_t MmGetMemoryDescriptorListSize(struct MmMemoryDescriptor *list)
{
    uint64_t size = 0;
    while(NULL != list)
    {
        size += list->size;
        list = list->next;
    }
    return size;
}

void *HalMapMemoryDescriptorList(struct MmMemoryDescriptor *list)
{
    if(NULL == list)
        return NULL;
    
    struct MmMemoryDescriptor first = list[0];
    uintptr_t offset = first.physical - ALIGN_DOWN(first.physical, PAGE_SIZE);
    first.physical -= offset;
    first.size += offset;
    first.size = ALIGN_UP(first.size, PAGE_SIZE);

    uint64_t size = 0;

    if(NULL != first.next)
        size = ALIGN_UP(MmGetMemoryDescriptorListSize(first.next) + first.size, PAGE_SIZE);
    else
        size = first.size;

    void *ret = MmReserveDynamicMemory(size);
    void *m = ret;
    if(NULL == m)
        return NULL;
    
    if(OK != HalMapMemoryEx((uintptr_t)m, first.physical, first.size, 0))
    {
        MmFreeDynamicMemoryReservation(ret);
        return NULL;
    }
    m = (void*)((uintptr_t)m + first.size);

    struct MmMemoryDescriptor *l = first.next;
    while(NULL != l)
    {
        if(OK != HalMapMemoryEx((uintptr_t)m, l->physical, l->size, 0))
        {
            //failure, unmap everything
            HalUnmapMemoryEx((uintptr_t)ret, first.size);
            struct MmMemoryDescriptor *k = list;
            m = ret;
            while(k != l)
            {
                HalUnmapMemoryEx((uintptr_t)m, k->size);
                k = k->next;
                m = (void*)((uintptr_t)m + k->size);
            }
            MmFreeDynamicMemoryReservation(ret);
            return NULL;
        }
        m = (void*)((uintptr_t)m + l->size);
        l = l->next;
    }

    return (void*)((uintptr_t)ret + offset);
}

void HalUnmapMemoryDescriptorList(void *memory)
{
    MmUnmapDynamicMemory(memory);
}

STATUS MmAllocateMemory(uintptr_t address, uintptr_t size, MmMemoryFlags flags)
{
    STATUS ret = OK;

    uintptr_t initialAddress = address;
    while(size)
    {
        if(address & (PAGE_SIZE - 1)) //check if memory is aligned
            return BAD_ALIGNMENT;
        
        uintptr_t pAddress = 0;
        uintptr_t allocated = MmAllocatePhysicalMemory(size, &pAddress);
        if(0 == allocated) //no memory was allocated - this is an error condition
        {
            ret = OUT_OF_RESOURCES;
            goto mmAllocateKernelMemoryFailed;
        }

        if(OK != (ret = HalMapMemoryEx(address, pAddress, allocated, flags)))
        {
            //mapping failed
            MmFreePhysicalMemory(pAddress, PAGE_SIZE); //free physical page that wasn't mapped
            goto mmAllocateKernelMemoryFailed;
        }
        address += allocated;
        size -= allocated;
    }

    return OK;

    mmAllocateKernelMemoryFailed:
    address -= PAGE_SIZE;
    uintptr_t pAddress = 0;
    //unmap and free previously mapped and allocated pages
    while(address != initialAddress)
    {
        HalGetPhysicalAddress(address, &pAddress);
        MmFreePhysicalMemory(pAddress, PAGE_SIZE);
        HalUnmapMemory(address);
        address -= PAGE_SIZE;
    }
    return ret;
}

STATUS MmAllocateMemoryZeroed(uintptr_t address, uintptr_t size, MmMemoryFlags flags)
{
    STATUS status = MmAllocateMemory(address, size, flags);
    if(OK == status)
        RtlMemset((void*)address, 0, size);
    return status;
}

STATUS MmFreeMemory(uintptr_t address, uintptr_t size)
{
    while(size)
    {
        uintptr_t pAddress = 0;
        if(OK == HalGetPhysicalAddress(address, &pAddress))
        {
            HalUnmapMemory(address);
            MmFreePhysicalMemory(pAddress, PAGE_SIZE);
        }
        address += PAGE_SIZE;
        if(size >= PAGE_SIZE)
            size -= PAGE_SIZE;
        else
            size = 0;
    }
    return OK;
}

