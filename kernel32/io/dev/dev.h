#ifndef KERNEL_DEV_H_
#define KERNEL_DEV_H_

#include <stdint.h>
#include "defines.h"
#include "bus.h"
#include "ob/ob.h"

EXPORT
#define IO_MAX_COMPATIBLE_DEVICE_IDS 8

EXPORT
typedef uint32_t IoDeviceFlags;
#define IO_DEVICE_FLAG_INITIALIZED 0x1
#define IO_DEVICE_FLAG_READY_TO_RUN 0x2
#define IO_DEVICE_FLAG_INITIALIZATION_FAILURE 0x4
#define IO_DEVICE_FLAG_FS_MOUNTED 0x8
#define IO_DEVICE_FLAG_FS_ASSOCIATED 0x10
#define IO_DEVICE_FLAG_BUFFERED_IO 0x1000
#define IO_DEVICE_FLAG_DIRECT_IO 0x2000
#define IO_DEVICE_FLAG_PERSISTENT 0x80000000
#define IO_DEVICE_FLAG_REMOVABLE_MEDIA 0x40000000
#define IO_DEVICE_FLAG_ENUMERATION_CAPABLE 0x20000000

EXPORT
enum IoDeviceType
{
    IO_DEVICE_TYPE_NONE = 0, //dummy driver
    IO_DEVICE_TYPE_OTHER, //other devices
    IO_DEVICE_TYPE_ROOT, //system root hardware (ACPI, MP)
    IO_DEVICE_TYPE_BUS, //bus (PCI, PCIe, ISA...)
    IO_DEVICE_TYPE_STORAGE, //storage controller (IDE, AHCI, NVMe...)
    IO_DEVICE_TYPE_DISK, //disk with partition manager (MBR, GPT)
    IO_DEVICE_TYPE_FS, //filesystem (EXT, FAT...)


    __IO_DEVICE_TYPE_COUNT, //count of device types, do not use
};

EXPORT
struct IoDeviceObject;
struct IoVolumeNode;
struct IoDeviceResource;
struct ExDeviceObject;
struct IoRp;

EXPORT
/**
 * @brief Maximum length of device name
*/
#define IO_DEVICE_MAX_NAME_LENGTH 64

EXPORT
/**
 * @brief Device node structure
*/
struct IoDeviceNode
{
    struct ObObjectHeader objectHeader;
    char name[IO_DEVICE_MAX_NAME_LENGTH + 1]; /**< User-friendly name of the device */
    IoDeviceFlags flags; /**< Common device flags */
    struct IoDeviceObject *bdo; /**< Base Device Object */
    struct IoDeviceObject *mdo; /**< Main Device Object */
    struct IoDeviceNode *parent; /**< Parent node */
    struct IoDeviceNode *child; /**< First child node */
    struct IoDeviceNode *next, *previous; /**< A list of nodes on the same level */
};

/**
 * @brief Device object structure
*/
EXPORT
struct IoDeviceObject
{
    struct ObObjectHeader objectHeader;
    enum IoDeviceType type; /**< Device type */
    void *privateData; /**< Private device data pointer */
    IoDeviceFlags flags; /**< Device flags */
    uint32_t alignment; /**< Required memory aligment for direct I/O */
    uint32_t blockSize; /**< Block size for direct I/O */

    struct ExDriverObject *driverObject; /**< Driver object */
    struct IoDeviceObject *nextDevice; /**< Linked list of devices created by the same driver */
    struct IoDeviceObject *attachedTo; /**< Device object to which this device is attached */
    struct IoDeviceObject *attachedDevice; /**< Next device object which is attached to this device */
    struct IoVolumeNode *associatedVolume; /**< Volume associated with disk device */
    uint32_t referenceCount; /**< Count of references to this device object */

    union
    {
        struct IoDeviceNode *node; /**< Device node this object is a part of */
        struct IoVolumeNode *volumeNode; /**< Parent volume node if this device is a filesystem device */
    };
};

EXPORT
/**
 * @brief Create device object from driver
 * @param *driver Driver object pointer
 * @param type Device type
 * @param flags Device flags
 * @param **device Output device object pointer. The memory is allocated by this function.
 * @return Status code
 * @attention If the device being created is not a BDO, then it must be attached to a stack using \a IoAttachDevice().
 * If it is a BDO, it must be registered in the system using \a IoRegisterDevice().
*/
EXTERN STATUS IoCreateDevice(
    struct ExDriverObject *driver, 
    enum IoDeviceType type,
    IoDeviceFlags flags, 
    struct IoDeviceObject **device);

EXPORT
/**
 * @brief Destroy device object
 * 
 * Destroy device object, e.g., on initialization failure.
 * This function fails when the device is already a part of a device stack.
 * @param *device Device to be destroyed
 * @return Status code
 * @attention This function destroys only unattached devices
*/
EXTERN STATUS IoDestroyDevice(struct IoDeviceObject *device);

EXPORT
/**
 * @brief Attach device to the top of the device stack
 * @param *attachee Device object to be attached
 * @param *destination Any device object in given device stack
 * @return Pointer to the previous top device stack object or NULL on failure
*/
EXTERN struct IoDeviceObject* IoAttachDevice(
    struct IoDeviceObject *attachee, 
    struct IoDeviceObject *destination);

EXPORT
/**
 * @brief Register new device to be used by the system
 * @param *bdo Base (Bus) Device Object
 * @param *enumerator The device which register the new device, aka enumerator
 * @return Status code
 * @warning This function should be called only by (or on behalf of) base (bus) subdevice object
*/
EXTERN STATUS IoRegisterDevice(
    struct IoDeviceObject *bdo, 
    struct IoDeviceObject *enumerator);

/**
 * @brief Build device stack (load drivers and add devices) for previously registered node
 * @param *node Devce node
 * @return Status code
*/
INTERNAL STATUS IoBuildDeviceStack(struct IoDeviceNode *node);

/**
 * @brief Initialize device manager and create root device
 * @param *rootDeviceId Root device ID string (to find appropriate driver)
 * @return Status code
*/
INTERNAL STATUS IoInitDeviceManager(char *rootDeviceId);

EXPORT
/**
 * @brief Send Request Packet to a given device or forward received RP to other device stack
 * @param *dev Device object
 * @param *rp RP to be sent
 * @return Status code
*/
EXTERN STATUS IoSendRp(struct IoDeviceObject *dev, struct IoRp *rp);

EXPORT
/**
 * @brief Send/pass Request Packet down the stack
 * @param *rp RP to be sent down
 * @return Status code
 * @attention This function fails if there are no more subdevices in the stack
*/
EXTERN STATUS IoSendRpDown(struct IoRp *rp);

EXPORT
/**
 * @brief Get main device ID and compatbile device IDs for a device
 * @param *dev Device object
 * @param **deviceId Main device ID. This buffer is allocated by the driver.
 * @param **compatbileIds[] List of compatbile IDs. This buffer is allocated by the driver.
 * @return Status code. On failure the returned IDs are NULL.
*/
EXTERN STATUS IoGetDeviceId(struct IoDeviceObject *dev, char **deviceId, char ***compatibleIds);

EXPORT
/**
 * @brief Read device configuration space
 * @param *dev Device which configuration space should be read
 * @param offset Offset into the configuration space in bytes
 * @param size Size in bytes
 * @param **buffer Output buffer. This buffer is allocated by the callee.
 * @return Status code
*/
EXTERN STATUS IoReadConfigSpace(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void **buffer);

EXPORT
/**
 * @brief Write device configuration space
 * @param *dev Device which configuration space should be written
 * @param offset Offset into the configuration space in bytes
 * @param size Size in bytes
 * @param *buffer Input buffer
 * @return Status code
*/
EXTERN STATUS IoWriteConfigSpace(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void *buffer);

EXPORT
/**
 * @brief Get resources associated with given device
 * @param *dev Device to get the resources of
 * @param **res Output buffer for resource list. Allocated by the callee.
 * @param *count Output number of resource entries
 * @return Status code
*/
EXTERN STATUS IoGetDeviceResources(struct IoDeviceObject *dev, struct IoDeviceResource **res, uint32_t *count);

EXPORT
/**
 * @brief Get device location information
 * @param *dev Device to find the location of
 * @param *type Output for bus type
 * @param *location Output for bus-specific location information
 * @return Status code
*/
EXTERN STATUS IoGetDeviceLocation(struct IoDeviceObject *dev, enum IoBusType *type, union IoBusId *location);

#endif