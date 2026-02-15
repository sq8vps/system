#include "llsyscall.h"
#include "mm/tmem.h"

#include "ke/core/panic.h"

#define __KERNEL_INTERNAL 1
#include "syscall.h"

void KeValidateUserReturnAddress(reg_t ret)
{
    if(!MmProbeUserMemory((const void*)ret, sizeof(void*), MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_EXECUTABLE))
    {
        //TODO: handle task termination
        KePanic(UNEXPECTED_FAULT);
    }
}

const struct KeSyscallDescriptor* KeGetSyscallDescriptor(size_t code)
{
    if(code >= (sizeof(KeSyscallDescriptorTable) / sizeof(KeSyscallDescriptorTable[0])))
        return nullptr;
    
    if(NULL == KeSyscallDescriptorTable[code])
        return nullptr;

    return KeSyscallDescriptorTable[code];
}

DEFINE_SYSCALL(STATUS, ApiNoOperation, uint32_t, uint16_t, uint8_t, void*, uint64_t);
STATUS ApiNoOperation(uint32_t unused1, uint16_t unused2, uint8_t unused3, void *unused4, uint64_t unused5)
{
    UNUSED(unused1);
    UNUSED(unused2);
    UNUSED(unused3);
    UNUSED(unused4);
    UNUSED(unused5);
    return OK;
}

DEFINE_SYSCALL(STATUS, ApiGetSystemConfig, enum ApiSystemConfigParam, uint64_t*);
STATUS ApiGetSystemConfig(enum ApiSystemConfigParam param, uint64_t *output)
{
    if(!MmProbeUserMemory(output, sizeof(*output), MM_TASK_MEMORY_READABLE))
        return BAD_PARAMETER;
    
    switch(param)
    {
        case API_SYSTEM_CONFIG_PAGE_SIZE:
            *output = PAGE_SIZE;
            break;
        default:
            return NOT_IMPLEMENTED;
    }

    return OK;
}