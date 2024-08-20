#include "attach.h"
#include "task.h"
#include "ke/sched/sched.h"
#include "ke/core/panic.h"
#include "hal/arch.h"

STATUS KeAttachToTask(struct KeTaskControlBlock *target, uint64_t timeout)
{
    // STATUS status = OK;
    // HalCheckPriorityLevel(HAL_PRIORITY_LEVEL_PASSIVE, HAL_PRIORITY_LEVEL_PASSIVE);

    // struct KeTaskControlBlock *source = KeGetCurrentTask();

    // if(PL_KERNEL != source->pl)
    //     KePanicIPEx(KE_GET_CALLER_ADDRESS(0), INVALID_TASK_ATTACHMENT_ATTEMPT, 2, (uintptr_t)source, (uintptr_t)target, 0);

    // if(target == source)
    //     KePanicIPEx(KE_GET_CALLER_ADDRESS(0), INVALID_TASK_ATTACHMENT_ATTEMPT, 0, (uintptr_t)source, (uintptr_t)target, 0);
    
    // if(NULL != source->attach.attachedTo)
    //     KePanicIPEx(KE_GET_CALLER_ADDRESS(0), INVALID_TASK_ATTACHMENT_ATTEMPT, 1, 
    //         (uintptr_t)source, (uintptr_t)target, (uintptr_t)source->attach.attachedTo);

    // if(!KeAcquireMutexWithTimeout(target->attach.mutex, timeout))
    //     return TIMEOUT;
    
    // target->attach.attachee = source;
    // source->attach.attachedTo = target;

    // status = HalAttachToTask(target);
    // if(OK != status)
    // {
    //     target->attach.attachee = NULL;
    //     source->attach.attachedTo = NULL;
    //     KeReleaseMutex(target->attach.mutex);
    // }
    // return status;
}