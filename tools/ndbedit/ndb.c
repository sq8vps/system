#include "ndb.h"
#include <string.h>
#include <stdlib.h>

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
    if(crc != (h->crc = NablaDbCrc32(h, h->size + sizeof(h))))
    {
        return false;
    }

    return true;
}

struct NablaDbEntry* NablaDbFind(struct NablaDbHeader *h, const char *name)
{
    struct NablaDbEntry *e = (struct NablaDbEntry*)(h + 1);
    uint32_t processed = 0;
    while((processed < h->size) && (e->type != NDB_END))
    {
        if(NABLADB_ARRAY_ELEMENT != (e->type & NABLADB_ARRAY_ELEMENT)) //standard entry
        {
            if(0 == strcmp(e->name, name))
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

bool NablaDbAppend(struct NablaDbHeader **d, const struct NablaDbEntry *entry)
{
    if(NABLADB_ARRAY_ELEMENT == (entry->type & NABLADB_ARRAY_ELEMENT))
        return false;

    if(entry->nameLength != (strlen(entry->name) + 1))
        return false;
    
    if(NDB_ARRAY != entry->type)
    {
        if(!NablaDbIsVariableLength(entry->type) && (NablaDbGetTypeLength(entry->type) != entry->dataLength))
            return false;
    }

    uint32_t requiredSize = sizeof(*entry) + entry->nameLength + NablaDbGetTotalPayloadSize(entry);

    *d = realloc(*d, (*d)->size + sizeof(**d) + requiredSize);
    if(NULL == *d)
        return false;

    struct NablaDbEntry *e = (struct NablaDbEntry*)((uintptr_t)(*d + 1) + (*d)->size - sizeof(*e)); //overwrite end tag
    
    memcpy(e, entry, requiredSize);
    (*d)->size += requiredSize;

    //append end tag
    e = (struct NablaDbEntry*)((uintptr_t)e + requiredSize);
    e->type = NDB_END;
    e->dataLength = 0;
    e->nameLength = 0;

    (*d)->crc = 0;
    (*d)->crc = NablaDbCrc32(*d, (*d)->size + sizeof(**d));

    return true;
}

bool NablaDbRemove(struct NablaDbHeader *h, struct NablaDbEntry *e)
{
    if(NABLADB_ARRAY_ELEMENT == (e->type & NABLADB_ARRAY_ELEMENT))
        return false;

    if(NDB_END == e->type)
        return false;

    uint32_t size = e->nameLength + NablaDbGetTotalPayloadSize(e);

    memmove(e, (void*)((uintptr_t)(e + 1) + size), h->size + sizeof(*h) - ((uintptr_t)e - (uintptr_t)h) - sizeof(*e) - size);

    h->size -= (sizeof(*e) + size);
    h->crc = 0;
    h->crc = NablaDbCrc32(h, h->size + sizeof(*h));

    return true;
}

bool NablaDbChangeName(struct NablaDbHeader **d, struct NablaDbEntry *e, const char *name)
{
    if(NABLADB_ARRAY_ELEMENT == (e->type & NABLADB_ARRAY_ELEMENT))
        return false;

    uint32_t newLength = strlen(name) + 1;
    if(1 == newLength)
        return false;
    
    if(e->nameLength == newLength)
    {

    }
    else
    {
        if(newLength > e->nameLength)
        {
            void *old = *d;

            *d = realloc(*d, (*d)->size + newLength - e->nameLength);
            if(NULL == *d)
                return false;

            e = (struct NablaDbEntry*)((intptr_t)e + ((intptr_t)(*d) - (intptr_t)old));
        }
        
        memmove((void*)((uintptr_t)(e + 1) + newLength), (void*)((uintptr_t)(e + 1) + e->nameLength), 
            (*d)->size + sizeof(**d) - ((uintptr_t)(e + 1) - (uintptr_t)*d + e->nameLength));

        if(newLength > e->nameLength)
            (*d)->size += (newLength - e->nameLength);
        else
            (*d)->size -= (e->nameLength - newLength);
        
        e->nameLength = newLength;
    }

    strcpy(e->name, name);
    (*d)->crc = 0;
    (*d)->crc = NablaDbCrc32(*d, (*d)->size + sizeof(**d));
    return true;
}

bool NablaDbChangeValue(struct NablaDbHeader **d, struct NablaDbEntry *e, union NablaDbData value, uint32_t length)
{
    if(NDB_END == e->type)
        return false;
    
    if(NDB_ARRAY == e->type)
        return false;

    bool isArrayElement = NABLADB_IS_ARRAY_ELEMENT(e->type);

    uint32_t dataLength = 0;
    uint8_t *v = NULL;
    if(isArrayElement)
    {
        dataLength = ((struct NablaDbArrayElement*)e)->dataLength;
        v = (void*)((struct NablaDbArrayElement*)e + 1);
    }
    else
    {
        dataLength = e->dataLength;
        v = (void*)((uintptr_t)(e + 1) + e->nameLength);
    }

    if(NablaDbIsVariableLength(e->type) && (length != dataLength))
    {
        if(length > dataLength)
        {
            void *old = *d;

            *d = realloc(*d, (*d)->size + length - dataLength);
            if(NULL == *d)
                return false;

            e = (struct NablaDbEntry*)((intptr_t)e + ((intptr_t)(*d) - (intptr_t)old));
            v = (uint8_t*)((intptr_t)v + ((intptr_t)(*d) - (intptr_t)old));
        }

        if(isArrayElement)
        {
            struct NablaDbArrayElement *t = (struct NablaDbArrayElement*)e;
            memmove((void*)((uintptr_t)(t + 1) + length), (void*)((uintptr_t)(t + 1) + dataLength), 
                (*d)->size + sizeof(**d) - ((uintptr_t)(t + 1) - (uintptr_t)*d + dataLength));
            
            t->dataLength = length;
        }
        else
        {
            memmove((void*)((uintptr_t)(e + 1) + e->nameLength + length), (void*)((uintptr_t)(e + 1) + e->nameLength + dataLength), 
                (*d)->size + sizeof(**d) - ((uintptr_t)(e + 1) - (uintptr_t)*d + e->nameLength + dataLength));

            e->dataLength = length;
        }

        if(length > dataLength)
            (*d)->size += (length - dataLength);
        else
            (*d)->size -= (dataLength - length);
        
        dataLength = length;
    }

    switch(NABLADB_TYPE_MASK(e->type))
    {
        case NDB_UTF8:
            memcpy(v, value.utf8, dataLength);
            break;
        case NDB_MULTI:
            memcpy(v, value.multi, dataLength);
            break;
        case NDB_UUID:
            memcpy(v, value.uuid, dataLength);
            break;
        default:
            memcpy(v, &(value.qword), dataLength);
            break;
    }


    (*d)->crc = 0;
    (*d)->crc = NablaDbCrc32(*d, (*d)->size + sizeof(**d));
    return true;
}

bool NablaDbWriteEmpty(struct NablaDbHeader **d)
{
    struct NablaDbEntry *e;
    *d = realloc(*d, sizeof(**d) + sizeof(*e));
    if(NULL == *d)
        return false;
    
    memcpy((*d)->magic, NABLADB_MAGIC, sizeof((*d)->magic));
    (*d)->crc = 0;
    (*d)->size = sizeof(*e);

    e = (struct NablaDbEntry*)(*d + 1);
    e->dataLength = 0;
    e->nameLength = 0;
    e->type = NDB_END;

    (*d)->crc = NablaDbCrc32(*d, sizeof(**d) + sizeof(*e));
    return true;
}

struct NablaDbEntry* NablaDbGetEntry(struct NablaDbHeader *h, struct NablaDbEntry *last)
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

bool NablaDbAppendArrayElement(struct NablaDbHeader **d, struct NablaDbEntry *array, struct NablaDbArrayElement *element)
{
    if(NDB_ARRAY != array->type)
        return false;
        
    if(!NABLADB_IS_ARRAY_ELEMENT(element->type))
        return false;

    if(!NablaDbIsVariableLength(element->type) && (NablaDbGetTypeLength(element->type) != element->dataLength))
        return false;

    void *old = *d;

    *d = realloc(*d, (*d)->size + sizeof(**d) + sizeof(*element) + element->dataLength);
    if(NULL == *d)
        return false;
    
    array = (struct NablaDbEntry*)((intptr_t)array + ((intptr_t)(*d) - (intptr_t)old));

    struct NablaDbArrayElement *t = (struct NablaDbArrayElement*)((uintptr_t)(array + 1) + array->nameLength);
    uint32_t i = 0;
    while(i < array->elementCount)
    {
        t = (struct NablaDbArrayElement*)((uintptr_t)(t + 1) + t->dataLength);
        ++i;
    }

    memmove((void*)((uintptr_t)t + sizeof(*element) + element->dataLength), 
        t,
        (*d)->size + sizeof(**d) - ((uintptr_t)t - (uintptr_t)(*d)));
    
    memcpy(t, element, sizeof(*element) + element->dataLength);

    ++array->elementCount;
    (*d)->size += sizeof(*element) + element->dataLength;
    (*d)->crc = 0;
    (*d)->crc = NablaDbCrc32(*d, (*d)->size + sizeof(**d));

    return true; 
}

bool NablaDbRemoveArrayElement(struct NablaDbHeader *h, struct NablaDbEntry *array, struct NablaDbArrayElement *element)
{
    if(NDB_ARRAY != array->type)
        return false;

    if(!NABLADB_IS_ARRAY_ELEMENT(element->type))
        return false;

    uint32_t size = element->dataLength + sizeof(*element);

    memmove(element, (void*)((uintptr_t)(element + 1) + element->dataLength), h->size + sizeof(*h) - ((uintptr_t)element - (uintptr_t)h) - size);

    --array->elementCount;
    h->size -= size;
    h->crc = 0;
    h->crc = NablaDbCrc32(h, h->size + sizeof(*h));

    return true;    
}

