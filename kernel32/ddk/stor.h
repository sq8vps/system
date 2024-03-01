#ifndef DDK_STOR_H_
#define DDK_STOR_H_

#include <stdint.h>
#include "defines.h"
#include "io/dev/dev.h"

EXPORT
/**
 * @brief Type specific operations for storage controller devices
*/
enum StorOperations
{
    STOR_NONE = 0,
    STOR_GET_GEOMETRY,
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
    
    uint64_t firstAddressableSector;
    uint64_t sectorCount;

    struct StorChs firstAddressableChs;
    struct StorChs lastAddressableChs;
    uint32_t cylinderCount;
    uint32_t tracksPerCylinder;
    uint32_t sectorsPerTrack;
};

EXPORT
/**
 * @brief Get disk device geometry
 * @param *target Target disk device BDO
 * @param **geometry Returned geometry structure, allocated by the driver
 * @return Status code
 * @attention This function is always synchronous
*/
EXTERN STATUS StorGetGeometry(struct IoDeviceObject *target, struct StorGeometry **geometry);


#endif