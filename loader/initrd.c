#include "initrd.h"
#include "common.h"
#include "mm.h"
#include <stddef.h>
#include "disp.h"

static struct InitrdHeader *header = NULL;
static uintptr_t initrdSize = 0;

error_t InitrdInit(uintptr_t address)
{
    error_t ret = OK;
    if(OK != (ret = Mm_allocateEx(address, 1, 0)))
        return ret;

    header = (struct InitrdHeader*)address;
    initrdSize = MM_PAGE_SIZE;
    memcpy(header->magic, INITRD_MAGIC_NUMBER, sizeof(INITRD_MAGIC_NUMBER));
    header->firstFileOffset = 0;

    return OK;
}

uintptr_t InitrdPrepareSpaceForFile(char *name, uintptr_t size)
{
    if(NULL == header)
        return 0;
    
    if(strlen(name) > INITRD_MAX_FILE_NAME_LENGTH)
        return 0;

    struct InitrdFileHeader *file = (struct InitrdFileHeader*)((uintptr_t)header + sizeof(*header));

    if(0 == header->firstFileOffset)
    {
        header->firstFileOffset = sizeof(*header);
        file = (struct InitrdFileHeader*)((uintptr_t)header + header->firstFileOffset);
    }
    else
    {
        while(file->nextFileOffset)
        {
            file = (struct InitrdFileHeader*)((uintptr_t)file + file->nextFileOffset);
        }
        file->nextFileOffset = file->size + sizeof(*file);
        file = (struct InitrdFileHeader*)((uintptr_t)file + file->nextFileOffset);
    }
    
    uintptr_t freeSpace = initrdSize - ((uintptr_t)file - (uintptr_t)header);

    size += sizeof(struct InitrdFileHeader);

    if(freeSpace < size)
    {
        error_t ret = OK;
        if(OK != (ret = Mm_allocateEx(ALIGN_UP((uintptr_t)file, MM_PAGE_SIZE), ALIGN_UP(size - freeSpace, MM_PAGE_SIZE) / MM_PAGE_SIZE, 0)))
        {
            file->nextFileOffset = 0;
            return 0;
        }
        initrdSize += ALIGN_UP(size - freeSpace, MM_PAGE_SIZE);
    }
    file->size = size - sizeof(struct InitrdFileHeader);
    memcpy(file->name, name, strlen(name) + 1);
    return (uintptr_t)(file + 1);
}

uintptr_t InitrdGetSize(void)
{
    return initrdSize;
}