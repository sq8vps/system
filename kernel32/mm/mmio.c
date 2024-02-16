#include "mmio.h"
#include "dynmap.h"

void *MmMapMmIo(uintptr_t pAddress, uintptr_t n)
{
    return MmMapDynamicMemory(pAddress, n, MM_PAGE_FLAG_PCD | MM_PAGE_FLAG_PWT);
}

void MmUnmapMmIo(void *ptr)
{
    MmUnmapDynamicMemory(ptr);
}