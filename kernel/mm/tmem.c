#include "tmem.h"
#include "ke/sched/sched.h"
#include "ke/core/mutex.h"
#include "mm/heap.h"
#include "io/fs/fs.h"
#include "mm/mm.h"
#include "rtl/string.h"
#include "ke/sys/llsyscall.h"
#include "hal/mm.h"

#if 1
#define BST_PROVIDE_ABSTRACTION
#include "rtl/bst.h"
#endif

#define MM_TASK_MEMORY_HEAP_FLAGS (MM_TASK_MEMORY_FIXED | MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE)

#define REGION(entry) ((struct MmTaskMemory*)(entry)->aux.v)
#define HAS_GUARD(entry) (entry->flags & MM_TASK_MEMORY_GROWABLE)

static uintptr_t MmGetRegionTop(const struct MmTaskMemory *m)
{
    uintptr_t top;
    top = (uintptr_t)m->end;
    if((m->flags & MM_TASK_MEMORY_GROWABLE) && (!(m->flags & MM_TASK_MEMORY_REVERSED)))
        top += PAGE_SIZE; //...include guard page
    return top;
}

static uintptr_t MmGetRegionBase(const struct MmTaskMemory *m)
{
    uintptr_t base;
    base = (uintptr_t)m->base;
    //if this memory has no explicitly defined base/top address or it is growable
    //and additionally it is a stack, then the guard page must be included
    if((m->flags & MM_TASK_MEMORY_GROWABLE) && (m->flags & MM_TASK_MEMORY_REVERSED))
        base -= PAGE_SIZE; //...include guard page
    return base;
}

static struct MmTaskMemory* MmFindDynamicMemoryBaseMarker(struct KeProcessControlBlock *pcb)
{
    struct MmTaskMemory *t = pcb->memory.tail;
    while(nullptr != t)
    {
        if(t->flags & MM_TASK_MEMORY_BASE_MARKER)
            break;
        t = t->previous;
    }
    return t;
}

static MmMemoryFlags MmTaskMemoryFlagsToMemoryFlags(enum MmTaskMemoryFlags flags)
{
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
    return mmFlags;
}

STATUS MmMapTaskMemory(void *address, size_t size, enum MmTaskMemoryFlags flags, int fd, size_t alignment, uint64_t offset, size_t limit, void **mapped)
{
    //TODO: implement proper file mapping
    STATUS status = OK;
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    size_t alignedSize = ALIGN_UP(size, PAGE_SIZE);
    struct MmTaskMemory *entry = NULL;
    struct IoFileHandle *file = NULL;
    bool guard = false;
    bool allocate = !!(flags & (MM_TASK_MEMORY_WRITABLE | MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_EXECUTABLE));

    if(NULL != mapped)
        *mapped = NULL;

    if(0 == size)
        return OK;
    
    if((fd >= 0) && (flags & MM_TASK_MEMORY_REVERSED))
        return BAD_PARAMETER;

    if(unlikely(flags & MM_TASK_MEMORY_BASE_MARKER))
        return BAD_PARAMETER;

    if(flags & MM_TASK_MEMORY_GROWABLE) //include guard page
    {
        alignedSize += PAGE_SIZE;
        guard = true;
    }

    if(flags & MM_TASK_MEMORY_FIXED)
    {
        if(NULL == address)
            return BAD_PARAMETER;
        
        if(((uintptr_t)address & (PAGE_SIZE - 1)) 
            || ((0 != alignment) && ((uintptr_t)address & (alignment - 1))))
            return BAD_ALIGNMENT;

        if(flags & MM_TASK_MEMORY_REVERSED)
        {
            if(alignedSize > (uintptr_t)address)
                return BAD_PARAMETER;
            
            address = (void*)((uintptr_t)address - alignedSize);
        }
    }
    else
    {
        if(0 != alignment)
            address = (void*)ALIGN_UP((uintptr_t)address, alignment);
        address = (void*)ALIGN_UP((uintptr_t)address, PAGE_SIZE);
    }

    if(!IS_USER_MEMORY(address, alignedSize))
        return BAD_PARAMETER;

    if((UINTPTR_MAX - (uintptr_t)address) < alignedSize)
        return BAD_PARAMETER;

    if(fd >= 0)
    {
        if(offset & (PAGE_SIZE - 1))
            return BAD_ALIGNMENT;

        ObLockObject(pcb);
        if(pcb->files.count >= (uint32_t)fd)
        {
            ObUnlockObject(pcb);
            KeAcquireMutex(&(pcb->files.table[fd].mutex));
            file = pcb->files.table[fd].handle;
            if(NULL == file)
            {
                KeReleaseMutex(&(pcb->files.table[fd].mutex));
                return NOT_FOUND;
            }
            
            //check if file is open in any correct mode (read, write, append)
            //and, when write through flag is specified, if it's open in write mode
            if((flags & MM_TASK_MEMORY_WRITE_THROUGH) && !(file->mode & IO_FILE_WRITE))
            {
                KeReleaseMutex(&(pcb->files.table[fd].mutex));
                return BUSY;
            }
        }
        else
        {
            ObUnlockObject(pcb);
            return NOT_FOUND;
        }
    }


    struct MmTaskMemory *previous = nullptr;

    KeAcquireMutex(&(pcb->memory.mutex));
    if(unlikely((NULL == pcb->memory.head) && !(flags & MM_TASK_MEMORY_FIXED)))
    {
        status = NOT_SUPPORTED;
        goto leave;
    }
    else
    {
        uintptr_t nextBase = 0;

        if(flags & MM_TASK_MEMORY_FIXED)
        {
            uintptr_t previousTop = 0;
            struct TreeNode *succeeding = TreeFindGreaterOrEqual(pcb->memory.tree, (uintptr_t)address);
            if(nullptr == succeeding) //no succeeding node?
            {
                nextBase = (uintptr_t)HAL_KERNEL_SPACE_BASE;

                struct TreeNode *t = TreeFindLess(pcb->memory.tree, (uintptr_t)address);
                if(nullptr == t) //no preceding node?
                    previousTop = (uintptr_t)pcb->memory.heapBase;
                else
                {
                    previous = REGION(t);
                    previousTop = MmGetRegionTop(REGION(t));
                }
            }
            else //there is a succeeding node
            {
                nextBase = MmGetRegionBase(REGION(succeeding));

                if(nullptr != REGION(succeeding)->previous) //there is a neighboring node with lower address
                {
                    previousTop = MmGetRegionTop(REGION(succeeding)->previous);
                    previous = REGION(succeeding)->previous;
                }
                else
                    previousTop = (uintptr_t)pcb->memory.heapBase;
            }

            uintptr_t end = (uintptr_t)address + alignedSize + (guard ? PAGE_SIZE : 0);
            if(((uintptr_t)address < previousTop) || (end > nextBase))
            {
                if(flags & MM_TASK_MEMORY_OVERRIDE)
                {
                    status = MmUnmapTaskMemory(address, end - (uintptr_t)address);
                    if(OK != status)
                        goto leave;
                    //unmapping alters the structure, "previous" is not valid anymore
                    struct TreeNode *t = TreeFindLess(pcb->memory.tree, (uintptr_t)address);
                    if(nullptr != t)
                        previous = REGION(t);
                }
                else
                {
                    //won't fit
                    status = OUT_OF_RESOURCES;
                    goto leave;
                }
            }
        }
        else //not MM_TASK_MEMORY_FIXED
        {
            uintptr_t potentialBase = 0;
            struct MmTaskMemory *marker = MmFindDynamicMemoryBaseMarker(pcb);
            struct MmTaskMemory *t = marker;
            if(nullptr == marker)
            {
                status = NOT_SUPPORTED;
                goto leave;
            }

            bool takeHint = 
                (nullptr != address) 
                && ((uintptr_t)address >= (uintptr_t)pcb->memory.heapBase) 
                && (((uintptr_t)address + alignedSize) < MmGetRegionBase(marker));
            
            if(takeHint)
            {
                struct TreeNode *n = TreeFindGreaterOrEqual(pcb->memory.tree, (uintptr_t)address + alignedSize);
                if(nullptr != n)
                    t = REGION(n);
                else
                    t = marker;
            }
            
retryWithoutHint:

            while(nullptr != t)
            {
                //'t' is the region following our potential new allocation
                previous = t->previous;
                nextBase = MmGetRegionBase(t);

                if(!takeHint)
                {
                    potentialBase = nextBase - alignedSize - (guard ? PAGE_SIZE : 0);
                    if(0 != alignment)
                        potentialBase = ALIGN_DOWN(potentialBase, alignment);

                    if(potentialBase >= ((nullptr != previous) ? MmGetRegionTop(previous) : (uintptr_t)pcb->memory.heapBase))
                    {
                        address = (void*)potentialBase;
                        break;
                    }
                }
                else
                {
                    potentialBase = (uintptr_t)address;
                    if((potentialBase >= ((nullptr != previous) ? MmGetRegionTop(previous) : (uintptr_t)pcb->memory.heapBase))
                        && ((potentialBase + alignedSize + (guard ? PAGE_SIZE : 0)) <= nextBase))
                    {
                        break;
                    }
                    else
                    {
                        takeHint = false;
                        goto retryWithoutHint;
                    }
                }
                t = previous;
            }
                
            if(nullptr == t)
            {
                status = OUT_OF_RESOURCES;
                goto leave;
            }
        }
    }

    entry = MmAllocateKernelHeapZeroed(sizeof(*entry));
    if(NULL == entry)
        goto leave;


    if(allocate)
    {
        if(flags & MM_TASK_MEMORY_REVERSED)
            status = MmAllocateMemoryZeroed((uintptr_t)(address + (guard ? PAGE_SIZE : 0)), alignedSize - (guard ? PAGE_SIZE : 0), MmTaskMemoryFlagsToMemoryFlags(flags));
        else
            status = MmAllocateMemoryZeroed((uintptr_t)address, alignedSize - (guard ? PAGE_SIZE : 0), MmTaskMemoryFlagsToMemoryFlags(flags));
    }

    if(OK != status)
        goto leave;

    if(NULL != file)
    {
        status = IoReadFileSync(fd, address, size, offset, NULL);
        if(OK != status)
            goto leave;

        if(flags & MM_TASK_MEMORY_WRITE_THROUGH)
            ATOMIC_ADD_FETCH(&(file->references), 1, ATOMIC_RELAXED);
    }

    entry->base = address;
    entry->end = (void*)((uintptr_t)address + alignedSize);
    entry->flags = flags & ~MM_TASK_MEMORY_OVERRIDE;
    entry->file = file;
    entry->offset = offset;
    entry->limit = limit;
    entry->allocated = allocate;
    ((struct TreeNode*)entry->treeData)->key = (uintptr_t)address;
    ((struct TreeNode*)entry->treeData)->aux.v = entry;
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

leave:
    if(NULL != file)
        KeReleaseMutex(&(pcb->files.table[fd].mutex));
    KeReleaseMutex(&(pcb->memory.mutex));
    if(OK != status)
        MmFreeKernelHeap(entry);
    return status;
}

static STATUS MmResizeTaskMemoryRegion(struct MmTaskMemory *entry, intptr_t bytes, enum MmTaskMemoryResizeMethod method)
{
    STATUS status = OK;

    bytes = ALIGN_UP(bytes, PAGE_SIZE);

    if(0 == bytes)
        return OK;

    if((nullptr == entry) || (entry->flags & MM_TASK_MEMORY_LOCKED) || (nullptr != entry->file))
    {
        status = BAD_PARAMETER;
        goto leave;
    }

    if((bytes < 0) && (((uintptr_t)entry->end - (uintptr_t)entry->base) < (uintptr_t)(-bytes)))
    {
        status = BAD_PARAMETER;
        goto leave;
    }

    if(((entry->flags & MM_TASK_MEMORY_REVERSED) && (MM_RESIZE_NORMAL == method)) || (MM_RESIZE_BOTTOM == method))
    {
        uintptr_t newBase = (uintptr_t)entry->base - bytes;
        if(bytes > 0)
        {
            if(unlikely((newBase > (uintptr_t)entry->base)))
            {
                status = BAD_PARAMETER;
                goto leave;
            }

            if((nullptr != entry->previous) && (MmGetRegionTop(entry->previous) > (newBase - (HAS_GUARD(entry) ? PAGE_SIZE : 0))))
            {
                status = OUT_OF_RESOURCES;
                goto leave;
            }

            if(entry->allocated)
            {
                status = MmAllocateMemory(newBase, bytes, MmTaskMemoryFlagsToMemoryFlags(entry->flags));
                if(OK != status)
                    goto leave;
            }
        }
        else
        {
            if(entry->allocated)
            {
                status = MmFreeMemory((uintptr_t)entry->base, -bytes);
                if(OK != status)
                    goto leave;
            }
        }
        entry->base = (void*)newBase;
        TREE_KEY(entry->treeData) = newBase;
    }
    else
    {
        uintptr_t newEnd = (uintptr_t)entry->end + bytes;
        if(bytes > 0)
        {
            if((newEnd > HAL_KERNEL_SPACE_BASE) || unlikely((newEnd < (uintptr_t)entry->end)))
            {
                status = BAD_PARAMETER;
                goto leave;
            }

            if((nullptr != entry->next) && (MmGetRegionBase(entry->next) < (newEnd + (HAS_GUARD(entry) ? PAGE_SIZE : 0))))
            {
                status = OUT_OF_RESOURCES;
                goto leave;
            }

            if(entry->allocated)
            {
                status = MmAllocateMemory((uintptr_t)entry->end, bytes, MmTaskMemoryFlagsToMemoryFlags(entry->flags));
                if(OK != status)
                    goto leave;
            }
        }
        else
        {
            if(entry->allocated)
            {
                status = MmFreeMemory(newEnd, -bytes);
                if(OK != status)
                    goto leave;
            }
        }
        entry->end = (void*)newEnd;
    }
leave:
    return status;    
}

STATUS MmResizeTaskMemory(const void *const ptr, intptr_t bytes, enum MmTaskMemoryResizeMethod method)
{
    STATUS status = OK;
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    struct MmTaskMemory *entry = nullptr;
    KeAcquireMutex(&(pcb->memory.mutex));

    entry = MmGetTaskMemoryDescriptor(ptr);

    if(nullptr != entry)
    {
        status = MmResizeTaskMemoryRegion(entry, bytes, method);
    }
    else
    {
        status = NOT_FOUND;
    }

    KeReleaseMutex(&(pcb->memory.mutex));
    return status;
}

STATUS MmUnmapTaskMemory(const void *const ptr, size_t length)
{
    STATUS status = NOT_FOUND;
    uintptr_t base = ALIGN_DOWN((uintptr_t)ptr, PAGE_SIZE);
    uintptr_t end = ALIGN_UP((uintptr_t)ptr + length, PAGE_SIZE);

    if(0 == length)
        return OK;

    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    KeAcquireMutex(&(pcb->memory.mutex));

    if(nullptr == pcb->memory.tree)
    {
        KeReleaseMutex(&(pcb->memory.mutex));
        return NOT_FOUND;
    }

    struct TreeNode *t = TreeFindGreaterOrEqual(pcb->memory.tree, base);
    if(nullptr == t)
    {
        KeReleaseMutex(&(pcb->memory.mutex));
        return NOT_FOUND;
    }

    struct MmTaskMemory *region = REGION(t); 
    while(nullptr != region)
    {
        if((base < MmGetRegionTop(region)) && (end > MmGetRegionBase(region)))
        {
            bool partial = false;
            if(region->flags & MM_TASK_MEMORY_LOCKED)
            {
                region = region->next;
                continue;
            }
            //partial unmapping
            //everything is page aligned, so if some address is not equal, there is at least one page spacing
            if(end < MmGetRegionTop(region))
            {
                status = MmResizeTaskMemoryRegion(region, -(end - MmGetRegionBase(region)), MM_RESIZE_BOTTOM);
                if(OK != status)
                    break;
                partial = true;
            }
            if(base > MmGetRegionBase(region))
            {
                status = MmResizeTaskMemoryRegion(region, -(MmGetRegionTop(region) - base), MM_RESIZE_TOP);
                if(OK != status)
                    break;
                partial = true;
            }

            if(partial)
            {
                region = region->next;
                continue;
            }

            struct MmTaskMemory *next = region->next;

            //remove from tree
            pcb->memory.tree = TreeRemove(pcb->memory.tree, (struct TreeNode*)region->treeData);
            //remove from list
            if(NULL != region->previous)
                region->previous->next = region->next;
            else
                pcb->memory.head = region->next;
            
            if(NULL != region->next)
                region->next->previous = region->previous;
            else
                pcb->memory.tail = region->previous;
            
            //free memory
            if(region->allocated)
                MmFreeMemory((uintptr_t)region->base, (size_t)((uintptr_t)region->end - (uintptr_t)region->base));
            
            if(NULL != region->file)
            {
                if(region->flags & MM_TASK_MEMORY_WRITE_THROUGH)
                    ATOMIC_SUB_FETCH(&(region->file->references), 1, ATOMIC_RELAXED);
            }

            //free descriptor
            MmFreeKernelHeap(region);
            region = next;
            status = OK;
        }
        else
            break;
    }
    KeReleaseMutex(&(pcb->memory.mutex));
    return status;
}

STATUS MmFreeAllProcessMemoryOnExit(struct KeProcessControlBlock *pcb)
{
    STATUS status = OK;
    
    struct MmTaskMemory *m = pcb->memory.head;
    while(nullptr != m)
    {
        if(nullptr != m->file)
        {
            //TODO: implement proper file flushing
        }
        if(m->allocated)
            HalFreeMemoryP(pcb, (uintptr_t)m->base, (uintptr_t)m->end - (uintptr_t)m->base);
        m = m->next;
        //TODO: entry freeing?
    }

    return status;
}

struct MmTaskMemory *MmGetTaskMemoryDescriptor(const void *const ptr)
{
    struct MmTaskMemory *ret = NULL;
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    KeAcquireMutex(&(pcb->memory.mutex));
    if(NULL != pcb->memory.tree)
    {
        struct TreeNode *t = TreeFindLessOrEqual(pcb->memory.tree, (uintptr_t)ptr);
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

bool MmProbeUserMemory(const void *ptr, size_t size, enum MmTaskMemoryFlags flags)
{
    while(1)
    {
        uintptr_t end = (uintptr_t)ptr + size;
        struct MmTaskMemory *m = MmGetTaskMemoryDescriptor(ptr);
        if(nullptr == m)
            return false;

        if((m->flags & flags) != flags)
            return false;
        
        if((uintptr_t)m->end >= end)
            return true;

        size -= ((uintptr_t)m->end - (uintptr_t)ptr);
        ptr = m->end;
    }

}

STATUS MmSetDynamicMemoryBase(void *address)
{
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    struct MmTaskMemory *entry = MmAllocateKernelHeapZeroed(sizeof(*entry));
    if(NULL == entry)
        return OUT_OF_RESOURCES;

    entry->base = address;
    entry->end = address;
    entry->flags = MM_TASK_MEMORY_BASE_MARKER | MM_TASK_MEMORY_LOCKED;
    ((struct TreeNode*)entry->treeData)->key = (uintptr_t)address;
    ((struct TreeNode*)entry->treeData)->aux.v = entry;
    
    struct MmTaskMemory *t = pcb->memory.head;
    uintptr_t a = (uintptr_t)address;
    if(nullptr == t)
    {
        pcb->memory.head = entry;
        pcb->memory.tail = entry;
        entry->previous = nullptr;
        entry->next = nullptr;
    }
    else
    {
        while(nullptr != t)
        {
            if(a <= MmGetRegionBase(t))
            {
                entry->next = t;
                if((nullptr != t->previous) && (a >= MmGetRegionTop(t->previous)))
                {
                    entry->previous = t->previous;
                    t->previous->next = entry;
                    t->previous = entry;
                }
                else if(nullptr != t->previous)
                {
                    t->previous = entry;
                    entry->previous = nullptr;
                    pcb->memory.head = entry;
                }
            }
            if(nullptr == t->next)
            {
                if(unlikely(MmGetRegionTop(t) < a))
                {
                    MmFreeKernelHeap(entry);
                    return BAD_PARAMETER;
                }
                t->next = entry;
                entry->previous = t;
                entry->next = nullptr;
                pcb->memory.tail = entry;
                break;
            }
            t = t->next;
        }
    }

    pcb->memory.tree = TreeInsert(pcb->memory.tree, (struct TreeNode*)entry->treeData);
    return OK;
}

DEFINE_SYSCALL(STATUS, ApiMapTaskMemory, void*, size_t, enum MmTaskMemoryFlags, int, size_t, uint64_t, size_t, void**)
STATUS ApiMapTaskMemory(void *address, size_t size, enum MmTaskMemoryFlags flags, int fd, size_t alignment, uint64_t offset, size_t limit, void **mapped)
{
    if((nullptr != mapped) && !MmProbeUserMemory(mapped, sizeof(*mapped), MM_TASK_MEMORY_WRITABLE))
        return BAD_PARAMETER;
    if(flags & MM_TASK_MEMORY_LOCKED)
        return BAD_PARAMETER;
    return MmMapTaskMemory(address, size, flags, fd, alignment, offset, limit, mapped);
}

DEFINE_SYSCALL(STATUS, ApiMapTaskMemoryA, void*, size_t, enum MmTaskMemoryFlags, void**)
STATUS ApiMapTaskMemoryA(void *address, size_t size, enum MmTaskMemoryFlags flags, void **mapped)
{
    if((nullptr != mapped) && !MmProbeUserMemory(mapped, sizeof(*mapped), MM_TASK_MEMORY_WRITABLE))
        return BAD_PARAMETER;
    if(flags & MM_TASK_MEMORY_LOCKED)
        return BAD_PARAMETER;
    return MmMapTaskMemory(address, size, flags, -1, 0, 0, 0, mapped);
}

DEFINE_SYSCALL(STATUS, ApiUnmapTaskMemory, const void *const, size_t)
STATUS ApiUnmapTaskMemory(const void *const ptr, size_t size)
{
    return MmUnmapTaskMemory(ptr, size);
}

DEFINE_SYSCALL(void*, ApiResizeHeap, intptr_t)
void *ApiResizeHeap(intptr_t increment)
{
    STATUS status = OK;
    struct KeProcessControlBlock *pcb = KeGetCurrentTaskParent();
    
    KeAcquireMutex(&(pcb->memory.mutex));

    void *current = pcb->memory.heap;
    if(0 != increment)
    {
        if((increment < 0) && (((uintptr_t)current - (uintptr_t)pcb->memory.heapBase) < (uintptr_t)(-increment)))
        {
            KeReleaseMutex(&(pcb->memory.mutex));
            return nullptr;
        }

        if(current != pcb->memory.heapBase)
            status = MmResizeTaskMemory(current, increment, MM_RESIZE_NORMAL);
        else
        {
            uintptr_t alignedBase = ALIGN_DOWN((uintptr_t)current, PAGE_SIZE);
            uintptr_t totalSize = (uintptr_t)pcb->memory.heapBase - alignedBase + increment;
            status = MmMapTaskMemory((void*)alignedBase, totalSize, MM_TASK_MEMORY_HEAP_FLAGS, -1, 0, 0, 0, nullptr);
        }

        if(OK == status)
        {
            pcb->memory.heap = ((void*)((uintptr_t)current + increment));
        }
        else
        {
            current = nullptr;
        }
    }

    KeReleaseMutex(&(pcb->memory.mutex));

    return current;
}