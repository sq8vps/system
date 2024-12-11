#include "tmem.h"
#include "ke/sched/sched.h"
#include "ke/core/mutex.h"
#include "mm/heap.h"
#include "io/fs/fs.h"
#include "mm/mm.h"
#include "rtl/string.h"

#if 1
#define BST_PROVIDE_ABSTRACTION
#include "rtl/bst.h"
#endif

#define REGION(node) ((struct MmTaskMemory*)(node)->main)

static uintptr_t MmGetRegionTop(const struct MmTaskMemory *m)
{
    uintptr_t top;
    top = (uintptr_t)m->end;
    //if this memory has no explicitly defined base/top address or it is growable
    //and additionally it is not a stack, then the guard page must be included
    if((!(m->flags & MM_TASK_MEMORY_FIXED) || (m->flags & MM_TASK_MEMORY_GROWABLE)) && (!(m->flags & MM_TASK_MEMORY_REVERSED)))
        top += PAGE_SIZE; //...include guard page
    return top;
}

static uintptr_t MmGetRegionBase(const struct MmTaskMemory *m)
{
    uintptr_t base;
    base = (uintptr_t)m->base;
    //if this memory has no explicitly defined base/top address or it is growable
    //and additionally it is a stack, then the guard page must be included
    if((!(m->flags & MM_TASK_MEMORY_FIXED) || (m->flags & MM_TASK_MEMORY_GROWABLE)) && (m->flags & MM_TASK_MEMORY_REVERSED))
        base -= PAGE_SIZE; //...include guard page
    return base;
}

STATUS MmMapTaskMemory(void *address, size_t size, enum MmTaskMemoryFlags flags, int fd, uint64_t offset, size_t limit, void **mapped)
{
    STATUS status = OK;
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    size_t alignedSize = ALIGN_UP(size, PAGE_SIZE);
    struct MmTaskMemory *entry = NULL;
    bool found = false;
    struct IoFileHandle *file = NULL;
    bool guard = false;

    if(NULL != mapped)
        *mapped = NULL;

    if(0 == size)
        return OK;
    
    if((fd >= 0) && (flags & MM_TASK_MEMORY_REVERSED))
        return OPERATION_NOT_ALLOWED;

    if(!(flags & MM_TASK_MEMORY_FIXED) || (flags & MM_TASK_MEMORY_GROWABLE)) //include guard page
    {
        alignedSize += PAGE_SIZE;
        guard = true;
    }

    if(flags & MM_TASK_MEMORY_FIXED)
    {
        if(NULL == address)
            return INVALID_ARGUMENT;
        
        if((uintptr_t)address & (PAGE_SIZE - 1))
            return BAD_ALIGNMENT;

        if(flags & MM_TASK_MEMORY_REVERSED)
        {
            if(alignedSize > (uintptr_t)address)
                return INVALID_ARGUMENT;
            
            address = (void*)((uintptr_t)address - alignedSize);

            if(!IS_USER_MEMORY(address, alignedSize))
                return INVALID_ARGUMENT;

            if((uintptr_t)address < (uintptr_t)pcb->memory.base)
                return INVALID_ARGUMENT;
        }
        else
        {
            if(!IS_USER_MEMORY(address, alignedSize))
                return INVALID_ARGUMENT;

            if((uintptr_t)address < (uintptr_t)pcb->memory.base)
                return INVALID_ARGUMENT;
        }
    }
    else
        address = (void*)ALIGN_UP((uintptr_t)address, PAGE_SIZE);

    if(fd >= 0)
    {
        if(offset & (PAGE_SIZE - 1))
            return BAD_ALIGNMENT;

        PRIO prio = ObLockObject(pcb);
        if(pcb->files.count >= fd)
        {
            ObUnlockObject(pcb, prio);
            KeAcquireMutex(&(pcb->files.table[fd].mutex));
            file = pcb->files.table[fd].handle;
            if(NULL == file)
            {
                KeReleaseMutex(&(pcb->files.table[fd].mutex));
                return FILE_NOT_FOUND;
            }
            
            //check if file is open in any correct mode (read, write, append)
            //and, when write through flag is specified, if it's open in write mode
            if(((flags & MM_TASK_MEMORY_WRITE_THROUGH) && !(file->mode & IO_FILE_WRITE))
                || (file->mode & IO_FILE_WRITE_ATTRIBUTES) || (file->mode & IO_FILE_READ_ATTRIBUTES))
            {
                KeReleaseMutex(&(pcb->files.table[fd].mutex));
                return FILE_BAD_MODE;
            }
        }
        else
        {
            ObUnlockObject(pcb, prio);
            return FILE_NOT_FOUND;
        }
    }


    struct MmTaskMemory *previous = NULL;
    uintptr_t nextBase = HAL_KERNEL_SPACE_BASE, 
              previousTop = (uintptr_t)pcb->memory.base;

    KeAcquireMutex(&(pcb->memory.mutex));
    if(NULL == pcb->memory.head)
    {
        if(NULL == address)
            address = pcb->memory.base;
    }
    else
    {
        if(flags & MM_TASK_MEMORY_FIXED)
        {
            struct TreeNode *t = TreeFindGreaterOrEqual(pcb->memory.tree, (uintptr_t)address);
            if(NULL == t) //no succeeding node?
            {
                nextBase = HAL_KERNEL_SPACE_BASE;

                t = TreeFindLess(pcb->memory.tree, (uintptr_t)address);
                if(NULL == t) //no preceding node?
                    previousTop = (uintptr_t)pcb->memory.base;
                else
                {
                    previous = REGION(t);
                    previousTop = MmGetRegionTop(REGION(t));
                }
            }
            else //there is a succeeding node
            {
                nextBase = MmGetRegionBase(REGION(t));

                if(NULL != REGION(t)->previous) //there is a neighboring node with lower address
                {
                    previousTop = MmGetRegionTop(REGION(t)->previous);
                    previous = REGION(t)->previous;
                }
                else
                    previousTop = (uintptr_t)pcb->memory.base;
            }

            if(((uintptr_t)address < previousTop) || (((uintptr_t)address + alignedSize) >= nextBase))
            {
                //won't fit
                status = OUT_OF_RESOURCES;
                goto MmMapTaskMemoryLeave;
            }
        }
        else //not MM_TASK_MEMORY_FIXED
        {
            //TODO: take hint from provided address?
            previousTop = (uintptr_t)pcb->memory.base;
            struct MmTaskMemory *t = pcb->memory.head;
            
            while(NULL != t)
            {
                nextBase = MmGetRegionBase(t);
                if((nextBase - previousTop) >= alignedSize)
                {
                    address = (void*)previousTop;
                    found = true;
                    break;
                }

                previousTop = MmGetRegionTop(t);
                previous = t;
                t = t->next;
            }

            //we end up here either because the gap is found (found = true) or because the end of the list was reached (found = false)
            if(!found)
            {
                if((HAL_KERNEL_SPACE_BASE - previousTop) >= alignedSize)
                {
                    address = (void*)previousTop;
                    found = true;
                }
                else
                {
                    status = OUT_OF_RESOURCES;
                    goto MmMapTaskMemoryLeave;
                }
            }
        }
    }

    entry = MmAllocateKernelHeapZeroed(sizeof(*entry));
    if(NULL == entry)
        goto MmMapTaskMemoryLeave;

    MmMemoryFlags mmFlags = 0;
    if(flags & MM_TASK_MEMORY_WRITABLE)
        mmFlags |= MM_FLAG_WRITABLE;
    else
        mmFlags |= MM_FLAG_READ_ONLY;
    if(flags & MM_TASK_MEMORY_EXECUTABLE)
        mmFlags |= MM_FLAG_EXECUTABLE; 
    else
        mmFlags |= MM_FLAG_NON_EXECUTABLE;
    mmFlags |= MM_FLAG_USER_MODE;

    if(flags & MM_TASK_MEMORY_REVERSED)
        status = MmAllocateMemoryZeroed((uintptr_t)(address + (guard ? PAGE_SIZE : 0)), alignedSize - (guard ? PAGE_SIZE : 0), mmFlags);
    else
        status = MmAllocateMemoryZeroed((uintptr_t)address, alignedSize - (guard ? PAGE_SIZE : 0), mmFlags);
    
    if(OK != status)
        goto MmMapTaskMemoryLeave;

    if(NULL != file)
    {
        status = IoReadFileSync(fd, address, size, offset, NULL);
        if(OK != status)
            goto MmMapTaskMemoryLeave;

        if(flags & MM_TASK_MEMORY_WRITE_THROUGH)
            __atomic_add_fetch(&(file->references), 1, __ATOMIC_RELAXED);
    }

    entry->base = address;
    entry->end = (void*)((uintptr_t)address + alignedSize);
    entry->flags = flags;
    entry->file = file;
    entry->offset = offset;
    entry->limit = limit;
    ((struct TreeNode*)entry->treeData)->key = (uintptr_t)address;
    ((struct TreeNode*)entry->treeData)->main = entry;
    if(NULL != previous)
    {
        entry->next = previous->next;
        if(NULL != entry->next)
            entry->next->previous = entry;
        else
            pcb->memory.tail = entry;
        entry->previous = previous;
        previous->next = entry;
    }
    else
    {
        if(NULL != pcb->memory.head)
            pcb->memory.head->previous = entry;
        else
            pcb->memory.tail = entry;
        entry->next = pcb->memory.head;
        pcb->memory.head = entry;
        entry->previous = NULL;
    }
    pcb->memory.tree = TreeInsert(pcb->memory.tree, (struct TreeNode*)entry->treeData);

    if(NULL != mapped)
    {
        if(flags & MM_TASK_MEMORY_REVERSED)
            *mapped = entry->end;
        else
            *mapped = entry->base;
    }

MmMapTaskMemoryLeave:
    if(NULL != file)
        KeReleaseMutex(&(pcb->files.table[fd].mutex));
    KeReleaseMutex(&(pcb->memory.mutex));
    if(OK != status)
        MmFreeKernelHeap(entry);
    return status;
}

struct MmTaskMemory *MmGetTaskMemoryDescriptor(const void *const ptr)
{
    struct MmTaskMemory *ret = NULL;
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    KeAcquireMutex(&(pcb->memory.mutex));
    if(NULL != pcb->memory.tree)
    {
        struct TreeNode *t = TreeFindGreaterOrEqual(pcb->memory.tree, (uintptr_t)ptr);
        if(NULL != t)
        {
            if(((uintptr_t)ptr >= (uintptr_t)REGION(t)->base) && ((uintptr_t)ptr < (uintptr_t)REGION(t)->end))
            {
                ret = REGION(t);
            }
        }
    }
    KeReleaseMutex(&(pcb->memory.mutex));
    return ret;
}