#include "mmio.h"
#include "dynmap.h"

void *MmMapMmIo(uintptr_t pAddress, uintptr_t n)
{
    return MmMapDynamicMemory(pAddress, n, MM_FLAG_WRITABLE | MM_FLAG_CACHE_DISABLE | MM_FLAG_WRITE_THROUGH);
}

void MmUnmapMmIo(void *ptr)
{
    MmUnmapDynamicMemory(ptr);
}