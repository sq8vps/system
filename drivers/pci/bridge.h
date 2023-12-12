#ifndef PCI_BRIDGE_H_
#define PCI_BRIDGE_H_

#include <stdint.h>
#include <stdbool.h>
#include "utils.h"
#include "defines.h"

struct PciBridge
{
    uint8_t primaryBus;
    uint8_t secondaryBus;
    uint8_t subordinateBus;
    struct PciAddress address;
    uint8_t level;
    uint8_t nextChildBus;
    
    struct PciBridge *parent;
    struct PciBridge *child;
    struct PciBridge *next;
};

STATUS PciRegisterBridge(struct PciAddress bridgeAddress, struct PciBridge *upstreamBridge, struct PciBridge **out);
STATUS PciRegisterHostBridge(struct PciAddress address, struct PciBridge **out);

#endif