#include "mmio.h"
#include "dynmap.h"

void *MmMapMmIo(PADDRESS pAddress, size_t n)
{
    return MmMapDynamicMemory(pAddress, n, MM_FLAG_WRITABLE | MM_FLAG_CACHE_DISABLE | MM_FLAG_WRITE_THROUGH);
}

void MmUnmapMmIo(const void *ptr)
{
    MmUnmapDynamicMemory(ptr);
}