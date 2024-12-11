#include "db.h"
#include "ndb.h"
#include "io/fs/fs.h"
#include "assert.h"
#include "mm/heap.h"
#include "rtl/string.h"

STATUS ExDbOpen(const char *path, struct ExDbHandle **h)
{
    ASSERT(h);
    STATUS status;

    int f = -1;
    uint64_t size;
    void *db = NULL;

    status = IoOpenFile(path, IO_FILE_READ, 0, &f);
    if(OK != status)
    {
        goto ExDbOpenFailed;
    }
    
    status = IoGetFileSize(path, &size);
    if(OK != status)
    {
        goto ExDbOpenFailed;
    }
    
    db = MmAllocateKernelHeap(size);
    if(NULL == db)
    {
        status = OUT_OF_RESOURCES;
        goto ExDbOpenFailed;
    }

    size_t actual = 0;
    status = IoReadFileSync(f, db, size, 0, &actual);
    if(actual != size)
    {
        status = READ_INCOMPLETE;
        goto ExDbOpenFailed;
    }
    else if(OK != status)
    {
        goto ExDbOpenFailed;
    }
    
    if(!NablaDbVerify(db))
    {
        status = DATABASE_BROKEN;
        goto ExDbOpenFailed;
    }

    *h = MmAllocateKernelHeapZeroed(sizeof(**h));
    if(NULL == *h)
    {
        status = OUT_OF_RESOURCES;
        goto ExDbOpenFailed;
    }

    (*h)->file = f;
    (*h)->db = db;
    (*h)->size = size;
    
    return OK;

ExDbOpenFailed:
    if(-1 != f)
        IoCloseFile(f);
    if(NULL != db)
        MmFreeKernelHeap(db);

    return status;
}

void ExDbClose(struct ExDbHandle *h)
{
    if(NULL != h)
    {
        IoCloseFile(h->file);
        MmFreeKernelHeap(h->db);
    }
    MmFreeKernelHeap(h); 
}

static struct NablaDbEntry* ExDbGetNext(struct ExDbHandle *h, const char *name, enum NablaDbType type)
{
    struct NablaDbEntry *e = NULL;
    if((NULL == h->last) 
        || (!NABLADB_IS_ARRAY_ELEMENT(h->last->type) && (0 != RtlStrcmp(h->last->name, name)))
        || ((NULL != h->array) && (0 != RtlStrcmp(h->array->name, name))))
    {
        e = NablaDbFind(h->db, name);
    }
    else if((NULL != h->array) && (!RtlStrcmp(h->array->name, name)))
        e = h->array;

    if(NULL == e)
    {
        h->array = NULL;
        h->last = NULL;
        return NULL;
    }

    if(NDB_ARRAY == e->type)
    {
        if(e == h->array)
            e = NablaDbGetEntry(h->db, h->last);
        else
        {
            h->array = e;
            e = NablaDbGetEntry(h->db, e);
        }

        while((NULL != e) && (NABLADB_IS_ARRAY_ELEMENT(e->type)))
        {
            h->last = e;
            if(type == NABLADB_TYPE_MASK(e->type))
            {
                return e;
            }
            e = NablaDbGetEntry(h->db, e);
        }
        h->array = NULL;
        h->last = NULL;
        return NULL;
    }
    else if(type == NABLADB_TYPE_MASK(e->type))
    {
        h->array = NULL;
        h->last = e;
        return e; 
    }

    h->last = NULL;
    h->array = NULL;

    return NULL;
}

STATUS ExDbGetNextString(struct ExDbHandle *h, const char *name, char **str)
{
    struct NablaDbEntry *e = ExDbGetNext(h, name, NDB_UTF8);
    if(NULL == e)
    {
        *str = NULL;
        return DATABASE_ENTRY_NOT_FOUND;
    }

    if(NABLADB_IS_ARRAY_ELEMENT(e->type))
    {
        *str = (char*)((struct NablaDbArrayElement*)e + 1);
    }
    else
    {
        *str = (char*)((uintptr_t)(e + 1) + e->nameLength);
    }

    return OK;
}

STATUS ExDbGetNextBool(struct ExDbHandle *h, const char *name, bool *b)
{
    struct NablaDbEntry *e = ExDbGetNext(h, name, NDB_BOOL);
    if(NULL == e)
    {
        return DATABASE_ENTRY_NOT_FOUND;
    }

    if(NABLADB_IS_ARRAY_ELEMENT(e->type))
    {
        *b = *((bool*)((struct NablaDbArrayElement*)e + 1));
    }
    else
    {
        *b = *((bool*)((uintptr_t)(e + 1) + e->nameLength));
    }

    return OK;
}

void ExDbRewind(struct ExDbHandle *h)
{
    h->array = NULL;
    h->last = NULL;
}