#include "ndb.h"
#include "common.h"

#define CRC32_POLYNOMIAL 0xEDB88320
#define CRC32_INITIAL 0xFFFFFFFF

static uint32_t NablaDbCrc32(const void *data, uint32_t size) 
{
    const uint8_t *d = data;
    uint32_t crc = CRC32_INITIAL;

    for(uint32_t i = 0; i < size; ++i)
    {
        crc = crc ^ d[i];
        for(int8_t k = 7; k >= 0; k--) 
        {
            crc = (crc >> 1) ^ (CRC32_POLYNOMIAL & (-(crc & 1)));
        }
    }
    return ~crc;
}

uint32_t NablaDbGetTotalPayloadSize(const struct NablaDbEntry *e)
{
    if(NABLADB_ARRAY_ELEMENT == (e->type & NABLADB_ARRAY_ELEMENT))
        return ((struct NablaDbArrayElement*)e)->dataLength;
    else if(NDB_ARRAY != e->type)
        return e->dataLength;
    
    uint32_t size = 0;
    struct NablaDbArrayElement *t = (struct NablaDbArrayElement*)((uintptr_t)(e + 1) + e->nameLength);
    uint32_t i = 0;
    while(i < e->elementCount)
    {
        size += sizeof(*t) + t->dataLength;
        t = (struct NablaDbArrayElement*)((uintptr_t)(t + 1) + t->dataLength);
        ++i;
    }
    return size;
}

bool NablaDbIsVariableLength(enum NablaDbType type)
{
    switch(NABLADB_TYPE_MASK(type))
    {
        case NDB_UTF8:
        case NDB_MULTI:
            return true;
            break;
        default:
            break;
    }

    return false;
}

uint32_t NablaDbGetTypeLength(enum NablaDbType type)
{
    switch(NABLADB_TYPE_MASK(type))
    {
        case NDB_END:
        case NDB_NULL:
            return 0;
            break;
        case NDB_BYTE:
        case NDB_BOOL:
            return 1;
            break;
        case NDB_WORD:
            return 2;
            break;
        case NDB_DWORD:
        case NDB_FLOAT:
            return 4;
            break;
        case NDB_QWORD:
        case NDB_TIMESTAMP:
        case NDB_DOUBLE:
            return 8;
            break;
        case NDB_UUID:
            return 16;
            break;
        case NDB_UTF8:
        case NDB_MULTI:
            return 0;
            break;
        default:
            return 0;
            break;
    }
}

bool NablaDbVerify(struct NablaDbHeader *h)
{
    for(uint8_t i = 0; i < sizeof(h->magic); i++)
    {
        if(h->magic[i] != NABLADB_MAGIC[i])
            return false;
    }
    
    uint32_t crc = h->crc;
    h->crc = 0;
    if(crc != (h->crc = NablaDbCrc32(h, h->size + sizeof(*h))))
    {
        return false;
    }

    return true;
}

struct NablaDbEntry* NablaDbFind(const struct NablaDbHeader *h, const char *name)
{
    struct NablaDbEntry *e = (struct NablaDbEntry*)(h + 1);
    uint32_t processed = 0;
    while((processed < h->size) && (e->type != NDB_END))
    {
        if(NABLADB_ARRAY_ELEMENT != (e->type & NABLADB_ARRAY_ELEMENT)) //standard entry
        {
            if(0 == CmStrcmp(e->name, name))
            {
                return e;
            }
            if(NDB_ARRAY != e->type)
            {
                e = (struct NablaDbEntry*)((uintptr_t)(e + 1) + e->nameLength + e->dataLength);
                processed += e->nameLength + e->dataLength + sizeof(*e);
            }
            else
            {
                e = (struct NablaDbEntry*)((uintptr_t)(e + 1) + e->nameLength);
                processed += e->nameLength + sizeof(*e);
            }
        }
        else //array element
        {
            struct NablaDbArrayElement *el = (struct NablaDbArrayElement*)e;
            e = (struct NablaDbEntry*)((uintptr_t)(el + 1) + el->dataLength);
            processed += el->dataLength + sizeof(*el);
        }
    }

    return NULL;
}

struct NablaDbEntry* NablaDbGetEntry(const struct NablaDbHeader *h, const struct NablaDbEntry *last)
{
    struct NablaDbEntry *e;
    if(NULL == last)
        e = (struct NablaDbEntry*)(h + 1);
    else
    {
        if(!NABLADB_IS_ARRAY_ELEMENT(last->type))
        {
            if(NDB_ARRAY != last->type)
                e = (struct NablaDbEntry*)((uintptr_t)(last + 1) + last->nameLength + last->dataLength);
            else
                e = (struct NablaDbEntry*)((uintptr_t)(last + 1) + last->nameLength);
        }
        else
        {
            struct NablaDbArrayElement *el = (struct NablaDbArrayElement*)last;
            e = (struct NablaDbEntry*)((uintptr_t)(el + 1) + el->dataLength);
        }
    }
    
    if(NDB_END != e->type)
        return e;
    else
        return NULL;
}