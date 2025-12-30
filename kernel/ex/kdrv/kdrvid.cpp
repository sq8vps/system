#include "templates/id.hpp"

#define EX_MAX_KERNEL_DRIVER_IDS 256

static IdDispenser<uint32_t, EX_MAX_KERNEL_DRIVER_IDS> ExKernelDriverIdDispenser; 

extern "C"
{
uint32_t ExAssignDriverId(void)
{
    return ExKernelDriverIdDispenser.assign();
}

void ExFreeDriverId(uint32_t id)
{
    ExKernelDriverIdDispenser.free(id);
}

}