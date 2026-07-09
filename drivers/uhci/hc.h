#ifndef UHCI_HC_H_
#define UHCI_HC_H_

#include "defines.h"
#include "if.h"

struct IoDeviceObject;

struct UhciControllerInfo
{
    bool isController; /**< Is this device a host controller? */
    struct IoDeviceObject *dev; /**< Associated device object */
    struct IoDeviceObject *bdo; /**< Base device object for host controller */
    uint16_t ioPort; /**< Host controller base I/O port */
    void *frameList; /**< Mapped frame list */
    size_t portCount; /**< Number of ports */
    bool controllerStarted; /**< Is controller running? */
    struct UsbHcd hcd; /**< USB HC data */
};

/**
 * @brief Configure newly detected UHCI controller
 * @param *bdo Base (enumerator) device object
 * @param *mdo Object for this device
 * @param *info Preallocated UHCI info structure
 * @return Status code
 */
STATUS UhciConfigureController(struct IoDeviceObject *bdo, struct IoDeviceObject *mdo, struct UhciControllerInfo *info);

/**
 * @brief Reset UHCI host controller
 * @param *hcd USB HC driver data
 * @return Status code
 */
STATUS UhciResetHostController(struct UsbHcd *hcd);

/**
 * @brief Start UHCI host controller
 * @param *hcd USB HC driver data
 * @return Status code
 */
STATUS UhciStartHostController(struct UsbHcd *hcd);

/**
 * @brief Stop UHCI host controller
 * @param *hcd USB HC driver data
 * @return Status code
 */
STATUS UhciStopHostController(struct UsbHcd *hcd);

/**
 * @brief Reset UHCI root hub port
 * @param *hcd USB HC driver data
 * @param port Port number (starting from 0)
 * @return Status code
 */
STATUS UhciResetPort(struct UsbHcd *hcd, size_t port);

/**
 * @brief Enable or disable UHCI root hub port
 * @param *hcd USB HC driver data
 * @param port Port number (starting from 0)
 * @param state New state - \a true to enable, \a false to disable
 * @return Status code
 */
STATUS UhciEnablePort(struct UsbHcd *hcd, size_t port, bool state);

/**
 * @brief Clear UHCI root hub port "enable changed" bit
 * @param *hcd USB HC driver data
 * @param port Port number (starting from 0)
 * @return Status code
 */
STATUS UhciClearPortEnableChange(struct UsbHcd *hcd, size_t port);

/**
 * @brief Clear UHCI root hub port "connect changed" bit
 * @param *hcd USB HC driver data
 * @param port Port number (starting from 0)
 * @return Status code
 */
STATUS UhciClearPortConnectChange(struct UsbHcd *hcd, size_t port);

/**
 * @brief Get UHCI root hub data
 * @param *s Pointer to the hub data structure to be filled
 * @return Status code
 */
STATUS UhciGetRootHubData(struct UsbHcd *hcd, struct UsbHcRootHubData *s);

/**
 * @brief Get UHCI root hub port status
 * @param port Port number (starting from 0)
 * @param *s Pointer to the port status structure to be filled
 * @return Status code
 */
STATUS UhciGetPortStatus(struct UsbHcd *hcd, size_t port, struct UsbRhPortStatus *s);

#endif