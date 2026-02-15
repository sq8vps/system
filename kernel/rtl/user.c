#include "string.h"
#include "mm/tmem.h"

size_t RtlStrlenUser(const char *str)
{
    size_t length = 0;
    const char *end = NULL;

    while(1)
    {
        end = (const char*)ALIGN_UP((uintptr_t)str, PAGE_SIZE);
        if(!MmProbeUserMemory(str, (size_t)(end - str), MM_TASK_MEMORY_READABLE))
            return (size_t)(-1);

        while(str != end)
        {
            if('\0' == *str)
                return length;
                
            ++length;
            ++str;
        }

    }

    return length;
}