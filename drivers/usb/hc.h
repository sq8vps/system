#ifndef UHCI_HC_H_
#define UHCI_HC_H_

#include "defines.h"

struct IoDeviceObject;

struct UhciControllerInfo
{
    bool isController; /**< Is this device a host controller? */
    struct IoDeviceObject *dev; /**< Associated device object */
    uint16_t ioPort; /**< Host controller base I/O port */
    void *frameList; /**< Mapped frame list */
    size_t portCount; /**< Number of ports */
    struct KeTaskControlBlock *worker; /**< UHCI worker thread */
};

/**
 * @brief Configure newly detected UHCI controller
 * @param *bdo Base (enumerator) device object
 * @param *mdo Object for this device
 * @param *info Preallocated UHCI info structure
 * @return Status code
 */
STATUS UhciConfigureController(struct IoDeviceObject *bdo, struct IoDeviceObject *mdo, struct UhciControllerInfo *info);

#endif