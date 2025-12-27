/**
 * @file stor.h
 * @brief Storage DDK and helpers
 * @ingroup ddk
 */

#ifndef DDK_STOR_H_
#define DDK_STOR_H_

#include <stdint.h>
#include "defines.h"

EXPORT_API

struct IoDeviceObject;

/**
 * @addtogroup ddk_stor Storage requests and helpers
 * @ingroup ddk
 * @note This module applies to storage controller devices, such as IDE or AHCI.
 * The module for abstract disk drives is disk.h.
 * @{
 */


/**
 * @brief Type specific operations for storage controller devices
*/
enum StorOperations
{
    STOR_NONE = 0, /**< No operation */
    STOR_GET_GEOMETRY = 1, /**< Get drive geometry */
};


/**
 * @brief Structure describing CHS address
*/
struct StorChs
{
    uint32_t head; /**< Head number */
    uint32_t cylinder; /**< Cylinder number */
    uint32_t sector; /**< Sector number */
};


/**
 * @brief Geometry structure for storage controller devices
*/
struct StorGeometry
{
    uint32_t sectorSize; /**< Sector size in bytes */
    
    uint64_t firstAddressableSector; /**< Number of the first addresable sector (LBA, starting from 0) */
    uint64_t sectorCount; /**< Number of addressable sectors */

    struct StorChs firstAddressableChs; /**< Number of the first addresable sector (CHS, starting from 1) */ 
    struct StorChs lastAddressableChs; /**< Number of the last addressable sector (CHS) */
    uint32_t cylinderCount; /**< Number of cylinders */
    uint32_t tracksPerCylinder; /**< Number of tracks per cylinder */
    uint32_t sectorsPerTrack; /**< Number of sectors per track */
};


/**
 * @brief Get disk device geometry
 * @param *target Target disk device BDO
 * @param **geometry Returned geometry structure, allocated by the driver
 * @return Status code
*/
STATUS StorGetGeometry(struct IoDeviceObject *target, struct StorGeometry **geometry);

/**
 * @}
 */

END_EXPORT_API

#endif