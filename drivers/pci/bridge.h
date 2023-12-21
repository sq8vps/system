#ifndef PCI_BRIDGE_H_
#define PCI_BRIDGE_H_

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include "defines.h"
#include "hal/interrupt.h"

struct PciBridge
{
    uint8_t primaryBus;
    uint8_t secondaryBus;
    uint8_t subordinateBus;
    union IoBusId address;
    uint8_t level;
    uint8_t nextChildBus;

    uint32_t irqCount;
    struct
    {
        union IoBusId address;
        uint8_t pin;
        uint32_t gsi;
        enum HalInterruptPolarity polarity;
        enum HalInterruptTrigger trigger;
        enum HalInterruptSharing sharing;
        enum HalInterruptWakeCapable wake; 
    } *irq;
    
    
    struct PciBridge *parent;
    struct PciBridge *child;
    struct PciBridge *next;
};

/**
 * @brief Register a PCI-PCI bridge
 * @param bridgeAddress PCI address of this bridge device
 * @param *upstream Upstream bridge object pointer
 * @param **out Output of created bridge object
 * @return Status code
*/
STATUS PciRegisterBridge(union IoBusId bridgeAddress, struct PciBridge *upstreamBridge, struct PciBridge **out);

/**
 * @brief Register a PCI host controller
 * @param bridgeAddress PCI address of this bridge device
 * @param **out Output of created bridge object
 * @return Status code
*/
STATUS PciRegisterHostBridge(union IoBusId address, struct PciBridge **out);

#endif