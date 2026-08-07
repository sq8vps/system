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
 * @brief USB endpoint type
 */
enum UsbEndpointType
{
    USB_EP_CONTROL = 0, /**< Control endpoint */
    USB_EP_ISOCHRONOUS = 1, /**< Isochronous endpoint */
    USB_EP_BULK = 2, /**< Bulk endpoint */
    USB_EP_INTERRUPT = 3, /**< Interrupt endpoint */
};

/**
 * @brief USB transfer direction
 */
enum UsbDirection
{
    USB_XFER_OUT = 0, /**< Host-to-device transfer */
    USB_XFER_IN = 1, /**< Device-to-host transfer */
};

/**
 * @brief USB endpoint configuration
 */
struct UsbEndpoint
{
    uint8_t deviceAddress; /**< Device address */
    enum UsbHcPortSpeed speed; /**< Device speed */
    uint8_t endpointAddress; /**< Endpoint number/address */
    enum UsbEndpointType type; /**< Endpoint type */
    size_t maxPacketSize; /**< Maximum packet size; this must be set accordingly to the USB specification */
    enum UsbDirection direction; /**< Endpoint direction */
    uint32_t period; /**< Polling period in ms, meaningful only when \a type is \ref USB_EP_INTERRUPT; must be set accordingly to the USB spec */
    /**
     * @brief Maximum anticipated total data size of a single request
     * 
     * This value must be set carefully. Too small values may result in unnecessary fragmentation,
     * while too big values may result in excessive memory usage.
     * When this value is incorrect (as per HC driver standards), it might be changed.
     * For isochronous pipes, this must be equal to \a maxPacketSize, as only 1 iso packet should be transmitted per pipe per frame.
     */
    size_t maxDataSize; 
};

/**
 * @brief USB transfer configuration
 */
struct UsbTransfer
{
    struct MmMemoryDescriptor *data; /**< Data buffer descriptors */
    enum UsbDirection direction; /**< Data direction for control transfers; ignored for other types */
};

/**
 * @brief USB HC interface
 */
struct UsbHcInterface
{
    char magic[sizeof(USB_HC_IF_MAGIC)]; /**< Identifier - magic number = \ref USB_HC_IF_MAGIC */
    enum UsbHcFlags flags; /**< Flags */
    enum UsbHcVersion version; /**< USB version */
    size_t transferContextSize; /**< Size of transfer context structure */

    STATUS (*InitializeHostController)(struct IoDeviceObject *bdo, struct IoDeviceObject *mdo, struct UsbHcd *hcd); /**< Initialize newly enumerated host controller */

    STATUS (*ResetHostController)(struct UsbHcd *hcd); /**< Reset host controller */
    STATUS (*StartHostController)(struct UsbHcd *hcd); /**< Start host controller */
    STATUS (*StopHostController)(struct UsbHcd *hcd); /**< Stop host controller */

    STATUS (*ResetRhPort)(struct UsbHcd *hcd, size_t port); /**< Reset root hub port */
    STATUS (*SetEnableRhPort)(struct UsbHcd *hcd, size_t port, bool state); /**< Enable or disable root hub port */
    STATUS (*ClearRhPortEnableChange)(struct UsbHcd *hcd, size_t port); /**< Clear "enable state changed" bit */
    STATUS (*ClearRhPortConnectChange)(struct UsbHcd *hcd, size_t port); /**< Clear "connect state changed" bit */
    STATUS (*GetRootHubData)(struct UsbHcd *hcd, struct UsbHcRootHubData *data); /**< Get root hub characteristics */
    STATUS (*GetRhPortStatus)(struct UsbHcd *hcd, size_t port, struct UsbRhPortStatus *status); /**< Get root hub port status */
};

#endif