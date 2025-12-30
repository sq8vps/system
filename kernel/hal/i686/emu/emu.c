#include "emu.h"
#include "state.h"
#include "ke/task/task.h"
#include "ke/sched/sched.h"
#include "mm/dynmap.h"
#include "rtl/string.h"
#include "hal/interrupt.h"

static struct I686EmuState I686EmulatorState = {.mutex = KeMutexInitializer, .dead = false};

enum I686EmulatorState I686EmulatorRun(struct I686EmuState *state);

STATUS I686InitializeEmulator(void)
{
    I686EmulatorState.code = MmMapDynamicMemory(0x0, I686_EMU_REAL_MODE_SPACE_SIZE, MM_FLAG_WRITE_THROUGH | MM_FLAG_CACHE_DISABLE);
    if(NULL == I686EmulatorState.code)
        return OUT_OF_RESOURCES;
    
    I686EmulatorState.ivt = (uint32_t*)I686EmulatorState.code;

    return OK;
}

STATUS I686AcquireEmulator(uint64_t timeout)
{
    if(!KeAcquireMutexEx(&I686EmulatorState.mutex, timeout))
        return TIMEOUT;
    
    I686EmulatorState.owner = KeGetCurrentTask();
    return OK;
}

STATUS I686AcquireEmulatorOnPanic(void)
{
    if(HalGetProcessorPriority() < HAL_PRIORITY_LEVEL_HIGHEST)
        return BAD_PARAMETER;
    
    I686EmulatorState.dead = true;
    return OK;
}

void I686ReleaseEmulator(void)
{
    if(I686EmulatorState.dead)
        return;
        
    if(KeGetCurrentTask() == I686EmulatorState.owner)
    {
        I686EmulatorState.owner = NULL;
        KeReleaseMutex(&I686EmulatorState.mutex);
    }
}

enum I686EmulatorState I686EmulatorDoInterrupt(uint8_t vector, struct I686Registers *regs, void *data, uint16_t size)
{
    if((KeGetCurrentTask() != I686EmulatorState.owner) && !I686EmulatorState.dead)
        return EMU_UNAVAILABLE;
    I686EmulatorState.registers = *regs;
    I686EmulatorState.registers.eflags |= 0b10;
    I686EmulatorState.registers.ss = (I686_EMULATOR_STACK_TOP & 0xF0000) >> 4;
    I686EmulatorState.registers.esp = I686_EMULATOR_STACK_TOP & 0xFFFF;
    if(0 != size)
        RtlMemcpy(I686EmulatorState.code + EMU_FAR_POINTER_TO_LINEAR(EMU_DATA_SEGMENT, EMU_DATA_OFFSET), data, size);
    I686EmulatorState.registers.cs = I686EmulatorState.ivt[vector] >> 16;
    I686EmulatorState.registers.eip = I686EmulatorState.ivt[vector] & 0xFFFF;

    enum I686EmulatorState state = EMU_CONTINUE;
    while(EMU_CONTINUE == state)
        state = I686EmulatorRun(&I686EmulatorState);
    
    *regs = I686EmulatorState.registers;
    
    if((EMU_OK == state) && (0 != size))
        RtlMemcpy(data, I686EmulatorState.code + EMU_FAR_POINTER_TO_LINEAR(EMU_DATA_SEGMENT, EMU_DATA_OFFSET), size);
    
    return state;
}

enum I686EmulatorState I686EmulatorReadMemory(uint32_t address, uint32_t size, void *buffer)
{
    if((KeGetCurrentTask() != I686EmulatorState.owner) && !I686EmulatorState.dead)
        return EMU_UNAVAILABLE;
    
    if(NULL == buffer)
        return EMU_MEMORY_VIOLATION;

    if(address > I686_EMU_REAL_MODE_SPACE_SIZE)
        return EMU_MEMORY_VIOLATION;
    
    if((I686_EMU_REAL_MODE_SPACE_SIZE - address) < size)
        return EMU_MEMORY_VIOLATION;
    
    RtlMemcpy(buffer, I686EmulatorState.code + address, size);

    return EMU_OK;
}