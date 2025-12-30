#ifndef NDB_H_
#define NDB_H_

#include <stdint.h>
#include <stdbool.h>

struct NablaDbHeader
{
    char magic[8];
    uint32_t size;
    uint32_t crc;
} __attribute__ ((packed));

struct NablaDbEntry
{
    uint8_t type;
    uint32_t nameLength;
    union
    {
        uint32_t dataLength;
        uint32_t elementCount;
    };
    char name[];
} __attribute__ ((packed));

struct NablaDbArrayElement
{
    uint8_t type;
    uint32_t dataLength;
    uint8_t value[];
} __attribute__ ((packed));

enum NablaDbType
{
    NDB_END = 0x00,
    NDB_NULL = 0x01,
    NDB_BYTE = 0x02,
    NDB_WORD = 0x03,
    NDB_DWORD = 0x04,
    NDB_QWORD = 0x05,
    NDB_BOOL = 0x06,
    NDB_UTF8 = 0x07,
    NDB_TIMESTAMP = 0x08,
    NDB_UUID = 0x09,
    NDB_FLOAT = 0x0A,
    NDB_DOUBLE = 0x0B,
    NDB_MULTI = 0x0C,

    NDB_ARRAY = 0x80,
    NDB_UNKNOWN = 0xFFFFFFFF,
};

#define NABLADB_MAGIC "_NABLADB"
#define NABLADB_ARRAY_ELEMENT (NDB_ARRAY | 0x40)
#define NABLADB_IS_ARRAY_ELEMENT(x) (NABLADB_ARRAY_ELEMENT == ((x) & NABLADB_ARRAY_ELEMENT))
#define NABLADB_TYPE_MASK(x) (x & ~NABLADB_ARRAY_ELEMENT)

union NablaDbData
{
    uint8_t byte;
    uint16_t word;
    uint32_t dword;
    uint64_t qword;
    uint8_t boolean;
    char *utf8;
    uint64_t timestamp;
    void *uuid;
    float fl;
    double db;
    uint8_t *multi;
};


bool NablaDbIsVariableLength(enum NablaDbType type);

uint32_t NablaDbGetTypeLength(enum NablaDbType type);

bool NablaDbVerify(struct NablaDbHeader *h);

struct NablaDbEntry* NablaDbFind(struct NablaDbHeader *h, const char *name);

bool NablaDbAppend(struct NablaDbHeader **d, const struct NablaDbEntry *entry);

bool NablaDbRemove(struct NablaDbHeader *h, struct NablaDbEntry *e);

bool NablaDbChangeName(struct NablaDbHeader **d, struct NablaDbEntry *e, const char *name);

bool NablaDbChangeValue(struct NablaDbHeader **d, struct NablaDbEntry *e, union NablaDbData value, uint32_t length);

bool NablaDbWriteEmpty(struct NablaDbHeader **d);

struct NablaDbEntry* NablaDbGetEntry(struct NablaDbHeader *h, struct NablaDbEntry *next);

bool NablaDbAppendArrayElement(struct NablaDbHeader **d, struct NablaDbEntry *array, struct NablaDbArrayElement *element);

bool NablaDbRemoveArrayElement(struct NablaDbHeader *h, struct NablaDbEntry *array, struct NablaDbArrayElement *element);

#endif