#include "mm.h"
#include "palloc.h"

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