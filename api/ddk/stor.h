//This header file is generated automatically
#ifndef EXPORTED___API__DDK_STOR_H_
#define EXPORTED___API__DDK_STOR_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"

struct IoDeviceObject;


/**
 * @brief Type specific operations for storage controller devices
*/
enum StorOperations
{
    STOR_NONE = 0,
    STOR_GET_GEOMETRY,
};


/**
 * @brief Structure describing CHS address
*/
struct StorChs
{
    uint32_t head;
    uint32_t cylinder;
    uint32_t sector;
};


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


/**
 * @brief Get disk device geometry
 * @param *target Target disk device BDO
 * @param **geometry Returned geometry structure, allocated by the driver
 * @return Status code
 * @attention This function is always synchronous
*/
STATUS StorGetGeometry(struct IoDeviceObject *target, struct StorGeometry **geometry);


#ifdef __cplusplus
}
#endif

#endif