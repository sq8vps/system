#ifndef DDK_STOR_H_
#define DDK_STOR_H_

#include <stdint.h>
#include "defines.h"
#include "io/dev/dev.h"
#include "io/dev/types.h"

EXPORT
/**
 * @brief Type specific operations for storage controller devices
*/
enum StorOperations
{
    STOR_NONE = 0,
    STOR_READ,
    STOR_WRITE,
    STOR_GET_GEOMETRY,
};

EXPORT
/**
 * @brief Transfer operation structure for storage controller devices
*/
struct StorTransfer
{
    struct IoDeviceObject *target;
    struct IoMemoryDescriptor *memory;
    void *buffer;
    uint64_t offset;
    uint64_t size;
    IoCompletionCallback completionCallback;
    void *completionContext;
    IoCancelRpCallback cancellationCallback;
};

EXPORT
/**
 * @brief Structure describing CHS address
*/
struct StorChs
{
    uint32_t head;
    uint32_t cylinder;
    uint32_t sector;
};

EXPORT
/**
 * @brief Geometry structure for storage controller devices
*/
struct StorGeometry
{
    uint32_t sectorSize;
    uint64_t sectorCount;
    uint64_t firstAddressableSector;
    struct StorChs chsGeometry;
    struct StorChs firstAddressableChs;
};



#endif