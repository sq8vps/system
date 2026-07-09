#ifndef USB_HC_IF_H_
#define USB_HC_IF_H_

#include "defines.h"

/**
 * @brief General USB Host Controller Driver data structure
 */
struct UsbHcd
{
    void *hcdData; /**< Low-level driver data */
};

/**
 * @brief Identifier field for \ref UsbHcInterface
 */
#define USB_HC_IF_MAGIC "USB    "

/**
 * @brief Flags for \ref UsbHcInterface
 */
enum UsbHcFlags
{
    USB_FLAG_DUMMY = 0,
};

/**
 * @brief USB bus version
 */
enum UsbHcVersion
{
    USB_VERSION_UNKNOWN = 0, /**< Unknown USB version */
    USB_VERSION_1 = 1, /**< USB 1.x */
    USB_VERSION_2 = 2, /**< USB 2.x */
};

/**
 * @brief USB port speed mode
 */
enum UsbHcPortSpeed
{
    USB_SPEED_UNKNOWN = 0, /**< Unknown USB speed mode */
    USB_LOW_SPEED = 1, /**< Low speed = 1.5 Mb/s */
    USB_FULL_SPEED = 2, /**< Full speed = 12 Mb/s */
    USB_HIGH_SPEED = 3, /**< High speed = 480 Mb/s (USB 2.0 only) */
};

/**
 * @brief USB root hub characteristics
 */
struct UsbHcRootHubData
{
    size_t portCount; /**< Number of ports */
};

/**
 * @brief USB root hub port status
 */
struct UsbRhPortStatus
{
    int connect : 1; /**< Device is connected */
    int enable : 1; /**< Port is enabled */
    int suspdend : 1; /**< Port is suspended. This is always zero if \a enabled is zero. */
    int reset : 1; /**< Port is in reset */
    int resume : 1; /**< Port resume is detected or driven by software */
    struct
    {
        int plus : 1; /**< D+ logical state */
        int minus : 1; /**< D- logical state */
    } lineStatus;
    enum UsbHcPortSpeed speed; /**< Speed mode */

    int connectChange : 1; /**< \a connect bit state changed */
    int enableChange : 1; /**< \a enable bit state changed */
    int suspendChange : 1; /**< \a suspend bit state changed */
    int resetChange : 1; /**< \a reset bit state changed */
};

/**
 * @brief USB HC interface
 */
struct UsbHcInterface
{
    char magic[sizeof(USB_HC_IF_MAGIC)]; /**< Identifier - magic number = \ref USB_HC_IF_MAGIC */
    enum UsbHcFlags flags; /**< Flags */
    enum UsbHcVersion version; /**< USB version */

    struct UsbHcd *hcd; /**< Device specific data */

    STATUS (*ResetHostController)(struct UsbHcd *hcd); /**< Reset host controller */
    STATUS (*StartHostController)(struct UsbHcd *hcd); /**< Start host controller */
    STATUS (*StopHostController)(struct UsbHcd *hcd); /**< Stop host controller */

    STATUS (*ResetRhPort)(struct UsbHcd *hcd, size_t port); /**< Reset root hub port */
    STATUS (*SetEnableRhPort)(struct UsbHcd *hcd, size_t port, bool state); /**< Enable or disable root hub port */
    STATUS (*ClearRhPortEnableChange)(struct UsbHcd *hcd, size_t port); /**< Clear "enable state changed" bit */
    STATUS (*ClearRhPortConnectChange)(struct UsbHcd *hcd, size_t port); /**< Clear "connect state changed" bit */
    STATUS (*GetRootHubData)(struct UsbHcd *hcd, struct UsbHcRootHubData *data); /**< Get root hub characteristics */
    STATUS (*GetRhPortStatus)(struct UsbHcd *hcd, size_t port, struct UsbHcPortStatus *status); /**< Get root hub port status */
};

#endif