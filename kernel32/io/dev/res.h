#ifndef DEV_RES_H_
#define DEV_RES_H_

#include <stdint.h>
#include "defines.h"
#include "bus.h"
#include "hal/interrupt.h"

EXPORT_API

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


/**
 * @brief Device resource types
*/
enum IoDeviceResourceType
{
    IO_RESOURCE_UNKNOWN = 0,
    IO_RESOURCE_IRQ,
    IO_RESOURCE_IRQ_MAP,
};


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


/**
 * @brief Copy IRQ map
 * @param *map Map starting point
 * @return Copied map
*/
struct IoIrqMap* IoCopyIrqMap(struct IoIrqMap *map);

END_EXPORT_API

#endif