#ifndef DEV_RES_H_
#define DEV_RES_H_

#include <stdint.h>
#include "defines.h"
#include "types.h"
#include "hal/interrupt.h"

EXPORT
/**
 * @brief IRQ entry
*/
struct IoIrqEntry
{
    union IoBusId id; /**< Device location on bus which this IRQ belongs to */
    uint32_t gsi; /**< Global System Interrupt/IRQ number*/
    uint32_t pin; /**< Interrupt pin, bus-specific */
    struct HalInterruptParams params; /**< IRQ parameters */
};

EXPORT
/**
 * @brief Special multi-level IRQ map structure for bus controllers
*/
struct IoIrqMap
{
    enum IoBusType type; /**< Type of bus this controller handles */
    union IoBusId id; /**< ID of the controller on parent bus */
    uint32_t irqCount; /**< Count of IRQs handled by the controller */
    struct IoIrqEntry *irq; /**< List of IRQs handled by the controller */
    struct IoIrqMap *next; /**< List of IRQ maps of sibling controllers */
    struct IoIrqMap *child; /**< Child controller IRQ map */
};

EXPORT
/**
 * @brief Device resource types
*/
enum IoDeviceResourceType
{
    IO_RESOURCE_UNKNOWN = 0,
    IO_RESOURCE_IRQ,
    IO_RESOURCE_IRQ_MAP,
};

EXPORT
/**
 * @brief Device resource descriptor
*/
struct IoDeviceResource
{
    enum IoDeviceResourceType type;
    union
    {
        /**
         * @brief \a IO_RESOURCE_IRQ
        */
        struct IoIrqEntry irq;
        
        /**
         * @brief \a IO_RESOURCE_IRQ_MAP
        */
        struct IoIrqMap irqMap;
    };
};

EXPORT
/**
 * @brief Copy IRQ map
 * @param *map Map starting point
 * @return Copied map
*/
EXTERN struct IoIrqMap* IoCopyIrqMap(struct IoIrqMap *map);

#endif