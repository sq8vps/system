#ifndef EX_NDB_H_
#define EX_NDB_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"

struct NablaDbHeader
{
    char magic[8];
    uint32_t size;
    uint32_t crc;
} PACKED;

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
} PACKED;

struct NablaDbArrayElement
{
    uint8_t type;
    uint32_t dataLength;
    uint8_t value[];
} PACKED;

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

/**
 * @brief Is entry with given type variable length?
 * @param type Input type
 * @return True if variable length, false otherwise
 */
bool NablaDbIsVariableLength(enum NablaDbType type);

/**
 * @brief Get data length associated with given type
 * @param type Input type
 * @return Data length associated with given type (0 for variable length types)
 */
uint32_t NablaDbGetTypeLength(enum NablaDbType type);

/**
 * @brief Verify Nabla database file
 * @param *h Nabla DB header pointer
 * @return True if verified successfully, false otherwise
 * @attention Whole database must bo loaded first
 */
bool NablaDbVerify(struct NablaDbHeader *h);

/**
 * @brief Find entry in Nabla database
 * @param *h Nabla DB header pointer
 * @param *name Name of the entry to be found
 * @return Found entry or NULL if not found
 */
struct NablaDbEntry* NablaDbFind(const struct NablaDbHeader *h, const char *name);

/**
 * @brief Get next entry in Nable database
 * @param *h Nabla DB header
 * @param *last Last entry pointer or NULL to start from the beginning
 * @return Next entry or NULL if end of database was reached
 */
struct NablaDbEntry* NablaDbGetEntry(const struct NablaDbHeader *h, const struct NablaDbEntry *last);


#endif