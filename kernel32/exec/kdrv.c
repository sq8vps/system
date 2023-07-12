#include "kdrv.h"
#include "exec.h"
#include "elf.h"
#include "../mm/heap.h"
#include "../common.h"
#include <stdbool.h>
#include "../ddk/gddk.h"
#include "../io/display.h"

#include "../../drivers/vga/vga.h"

/**
 * @brief Union for structures of callback for any driver type
*/
typedef union
{
    GDDK_KDrvCallbacks_t graphics;
} KDrv_Callbacks_t;



#define KDRV_INITIAL_INDEX 0
KDRV_INDEX_T nextDriverIndex = KDRV_INITIAL_INDEX;

struct KDrv_DriverListEntry
{
    KDRV_INDEX_T index; //driver index
    DDK_KDrvMetadata_t *meta; //driver metadata
    void (*entry)(KDRV_INDEX_T index); //driver entry point (only for initialization)
    uintptr_t *vaddr; //driver virtual address
    uintptr_t size; //driver memory size (heap not included)
    uint8_t started : 1; //is driver started?

    DDK_KDrvGeneralCallbacks_t *generalCallbacks; //general driver callbacks
    KDrv_Callbacks_t *specializedCallbacks; //specialized driver callbacks
    
    struct KDrv_DriverListEntry *next; //next driver in linked list
};

struct KDrv_DriverListEntry *kernelModeDriverList = NULL;

static void ex_initKernelModeDriverEntry(struct KDrv_DriverListEntry *e)
{
    e->entry = NULL;
    e->vaddr = NULL;
    e->size = 0;
    e->started = 0;
    e->meta = NULL;
    e->index = 0;
    e->generalCallbacks = NULL;
}

static struct KDrv_DriverListEntry* ex_getLastKernelModeDriverEntry(void)
{
    struct KDrv_DriverListEntry *t = kernelModeDriverList;
    if(NULL == t)
        return NULL;

    while(NULL != t->next)
    {
        t++;
    }

    return t;
}

static struct KDrv_DriverListEntry* ex_getKernelModeDriverEntry(const KDRV_INDEX_T index)
{
    struct KDrv_DriverListEntry *t = kernelModeDriverList;
    if(NULL == t)
        return NULL;

    while(index != t->index)
    {
        if(NULL == t->next)
            return NULL;
        t++;
    }

    return t;
}

static STATUS ex_pushDriverMetadata(const struct KDrv_DriverListEntry *e)
{
    struct KDrv_DriverListEntry *m = MmAllocateKernelHeap(sizeof(struct KDrv_DriverListEntry)); //allocate memory for entry
    if(NULL == m)
        return EXEC_MALLOC_FAILED;
    
    Cm_memcpy(m, e, sizeof(*m)); //copy entry

    m->next = NULL; //just to be sure

    if(NULL == kernelModeDriverList) //initialize head if not initialized
    {
        kernelModeDriverList = m;
    }
    else //head initialized, update last entry
    {
        ex_getLastKernelModeDriverEntry()->next = m;
    }
    
    return OK;
}

STATUS Ex_loadPreloadedDriver(const uintptr_t driverImage, const uintptr_t size)
{
    STATUS ret = OK;

    struct Elf32_Ehdr *h = (struct Elf32_Ehdr*)driverImage; //get ELF header

    ret = Ex_verifyElf32Header(h); //verify header
    if(OK != ret)
        return ret;

    if(ET_REL != h->e_type) //only relocatable drivers are supported
        return EXEC_UNSUPPORTED_KERNEL_MODE_DRIVER_TYPE;

    ret = Ex_performElf32Relocation(h, Ex_getKernelSymbol); //perform relocation
    if(OK != ret)
        return ret;

    uintptr_t entry = 0;
    ret = Ex_getElf32SymbolValueByName(h, EXPAND_AND_STRINGIFY(KDRV_ENTRY_NAME), &entry); //get entry point location
    if(OK != ret)
        return ret;
    
    uintptr_t meta = 0;
    ret = Ex_getElf32SymbolValueByName(h, EXPAND_AND_STRINGIFY(KDRV_METADATA_NAME), &meta); //get metadata location
    if(OK != ret)
        return ret;

    //store driver metadata
    struct KDrv_DriverListEntry m;
    ex_initKernelModeDriverEntry(&m);
    m.vaddr = (uintptr_t*)driverImage;
    m.size = size;
    m.meta = (DDK_KDrvMetadata_t*)meta;
    m.entry = (void(*)(KDRV_INDEX_T))entry;
    m.index = nextDriverIndex;
    ret = ex_pushDriverMetadata(&m);
    if(OK != ret)
        return ret;

    nextDriverIndex++;

    m.entry(m.index); //call driver entry (initialize driver)
    
    ex_getKernelModeDriverEntry(m.index)->generalCallbacks->start();

    return OK;
}

STATUS Ex_registerDriverGeneralCallbacks(const KDRV_INDEX_T index, const DDK_KDrvGeneralCallbacks_t *callbacks)
{
    struct KDrv_DriverListEntry *e = ex_getKernelModeDriverEntry(index);
    if(NULL == e)
        return EXEC_BAD_DRIVER_INDEX;
    if(NULL == callbacks)
        return NULL_POINTER_GIVEN;

    e->generalCallbacks = (DDK_KDrvGeneralCallbacks_t*)callbacks;
    return OK;
}

STATUS Ex_registerDriverCallbacks(const KDRV_INDEX_T index, const void *callbacks)
{
    struct KDrv_DriverListEntry *e = ex_getKernelModeDriverEntry(index);
    if(NULL == e)
        return EXEC_BAD_DRIVER_INDEX;

    printf("Registering callbacks\n");
    
    e->specializedCallbacks = (KDrv_Callbacks_t*)callbacks; //store callback structure

    switch(e->meta->class) //select class
    {
        case DDK_KDRVCLASS_SCREEN: //screen driver class
            printf("Graphics callback\n");
            Io_displaySetCallbacks(callbacks);
            break;
        default: //bad driver class
            return EXEC_BAD_DRIVER_CLASS;
            break;
    }

    return OK;
}