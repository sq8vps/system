#ifndef KERNEL_DEV_H_
#define KERNEL_DEV_H_

#include <stdint.h>
#include "defines.h"
#include "bus.h"
#include "ob/ob.h"

EXPORT_API

#define IO_MAX_COMPATIBLE_DEVICE_IDS 8

/**
 * @brief Device status
 */
enum IoDeviceStatus
{
    IO_DEVICE_STATUS_UNINITIALIZED = 0, /**< Device freshly created */
    IO_DEVICE_STATUS_READY, /**< Device initialized and ready */
    IO_DEVICE_STATUS_INITIALIZATION_FAILED, /**< Initialization failed - the device cannot be used */
};

/**
 * @brief Device status flags
 */
enum IoDeviceStatusFlags
{
    IO_DEVICE_STATUS_FLAG_FS_REGISTERED = 0x1, /**< Filesystem is registered, that is, associated with disk device */
    IO_DEVICE_STATUS_FLAG_FS_MOUNTED = 0x2, /**< Filesystem is mounted to some mount point */
    IO_DEVICE_STATUS_FLAG_ENUMERATION_FAILED = 0x4, /**< Enumeration failed on the device */
};

/**
 * @brief Device flags
 */
enum IoDeviceFlags
{
    IO_DEVICE_FLAG_BUFFERED_IO = 0x1, /**< Device support buffered I/O */
    IO_DEVICE_FLAG_DIRECT_IO = 0x2, /**< Device support direct I/O */
    IO_DEVICE_FLAG_PERSISTENT = 0x4, /**< Device is persistent and cannot be removed */
    IO_DEVICE_FLAG_REMOVABLE_MEDIA = 0x8, /**< Device is a removable media */
    IO_DEVICE_FLAG_ENUMERATION_CAPABLE = 0x10, /**< Device is capable of enumerating its children devices */
    IO_DEVICE_FLAG_NO_AUTOMOUNT = 0x20, /**< Filesystem device must not be automatically mounted */
    IO_DEVICE_FLAG_STANDALONE = 0x40, /**< Device is standalone and has no parent or enumerator */
    IO_DEVICE_FLAG_HIDDEN = 0x80, /**< Device is hidden */
};

enum IoDeviceType
{
    IO_DEVICE_TYPE_NONE = 0, //dummy driver
    IO_DEVICE_TYPE_OTHER, //other devices
    IO_DEVICE_TYPE_ROOT, //system root hardware (ACPI, ...)
    IO_DEVICE_TYPE_BUS, //bus (PCI, PCIe, ISA...)
    IO_DEVICE_TYPE_STORAGE, //storage controller (IDE, AHCI, NVMe...)
    IO_DEVICE_TYPE_DISK, //disk with partition manager (MBR, GPT)
    IO_DEVICE_TYPE_FS, //filesystem (EXT, FAT...)
    IO_DEVICE_TYPE_TERMINAL, //terminal


    __IO_DEVICE_TYPE_COUNT, //count of device types, do not use
};


struct IoDeviceObject;
struct IoVolumeNode;
struct IoDeviceResource;
struct ExDeviceObject;
struct IoRp;
struct IoVfsNode;


/**
 * @brief Maximum length of device name
*/
#define IO_DEVICE_MAX_NAME_LENGTH 64


/**
 * @brief Device node structure
*/
struct IoDeviceNode
{
    struct ObObjectHeader objectHeader;
    char name[IO_DEVICE_MAX_NAME_LENGTH + 1]; /**< User-friendly name of the device */
    enum IoDeviceStatus status; /**< Device state */
    enum IoDeviceStatusFlags statusFlags; /**< Additional device status flags */
    bool standalone; /**< Device is standalone - no parent, no enumerator */
    struct IoDeviceObject *bdo; /**< Base Device Object */
    struct IoDeviceObject *mdo; /**< Main Device Object */
    struct IoDeviceNode *parent; /**< Parent node */
    struct IoDeviceNode *child; /**< First child node */
    struct IoDeviceNode *next, *previous; /**< A list of nodes on the same level */
};

/**
 * @brief Device object structure
*/

struct IoDeviceObject
{
    OBJECT;
    enum IoDeviceType type; /**< Device type */
    void *privateData; /**< Private device data pointer */
    enum IoDeviceFlags flags; /**< Device flags */
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
        struct IoDeviceNode *deviceNode; /**< Device node this object is a part of */
        struct IoVolumeNode *volumeNode; /**< Parent volume node if this device is a filesystem device */
    } node;
};


/**
 * @brief Get memory alignment required to perform direct I/O
 * @param dev Target device
 * @return Required alignment in bytes
 * @warning Might return 0, which should be treated as 1 (any alignment)
 */
#define IO_DEV_REQUIRED_ALIGNMENT(dev) (((dev)->blockSize > (dev)->alignment) ? (dev)->blockSize : (dev)->alignment) 


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
STATUS IoCreateDevice(
    struct ExDriverObject *driver, 
    enum IoDeviceType type,
    enum IoDeviceFlags flags, 
    struct IoDeviceObject **device);


/**
 * @brief Destroy device object
 * 
 * Destroy device object, e.g., on initialization failure.
 * This function fails when the device is already a part of a device stack.
 * @param *device Device to be destroyed
 * @return Status code
 * @attention This function destroys only unattached devices
*/
STATUS IoDestroyDevice(struct IoDeviceObject *device);


/**
 * @brief Attach device to the top of the device stack
 * @param *attachee Device object to be attached
 * @param *destination Any device object in given device stack
 * @return Pointer to the previous top device stack object or NULL on failure
*/
struct IoDeviceObject* IoAttachDevice(
    struct IoDeviceObject *attachee, 
    struct IoDeviceObject *destination);


/**
 * @brief Register new device to be used by the system
 * @param *bdo Base (Bus) Device Object
 * @param *enumerator The device which register the new device, aka enumerator
 * @return Status code
 * @warning This function should be called only by (or on behalf of) base (bus) subdevice object
*/
STATUS IoRegisterDevice(
    struct IoDeviceObject *bdo, 
    struct IoDeviceObject *enumerator);

/**
 * @brief Register new standalone device to be used by the system
 * @param *dev Device object
 * @return Status code
*/
STATUS IoRegisterStandaloneDevice(struct IoDeviceObject *dev);

/**
 * @brief Destroy device node
 * @param *node Device node
 * @return Status code
 * @attention It is the caller's responsibility to make sure that destroying the device node is safe
 */
STATUS IoDestroyDeviceNode(struct IoDeviceNode *node);

/**
 * @brief Send Request Packet to a given device or forward received RP to other device stack
 * @param *dev Device object
 * @param *rp RP to be sent
 * @return Status code
*/
STATUS IoSendRp(struct IoDeviceObject *dev, struct IoRp *rp);

/**
 * @brief Send/pass Request Packet down the stack
 * @param *rp RP to be sent down
 * @return Status code
 * @attention This function fails if there are no more subdevices in the stack
*/
STATUS IoSendRpDown(struct IoRp *rp);

/**
 * @brief Perform IOCTL on a device
 * @param *dev Device object
 * @param ioctl IOCTL code (driver-specific)
 * @param *dataIn Input data for IOCTL
 * @param **dataOut Output data buffer for IOCTL. Invalid on status != OK.
 * @return Status code
 */
STATUS IoPerfromIoctl(struct IoDeviceObject *dev, uint32_t ioctl, void *dataIn, void **dataOut);

/**
 * @brief Get main device ID and compatbile device IDs for a device
 * @param *dev Device object
 * @param **deviceId Main device ID. This buffer is allocated by the driver.
 * @param **compatbileIds[] List of compatbile IDs. This buffer is allocated by the driver.
 * @return Status code. On failure the returned IDs are NULL.
*/
STATUS IoGetDeviceId(struct IoDeviceObject *dev, char **deviceId, char ***compatibleIds);


/**
 * @brief Read device configuration space
 * @param *dev Device which configuration space should be read
 * @param offset Offset into the configuration space in bytes
 * @param size Size in bytes
 * @param **buffer Output buffer. This buffer is allocated by the callee.
 * @return Status code
*/
STATUS IoReadConfigSpace(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void **buffer);


/**
 * @brief Write device configuration space
 * @param *dev Device which configuration space should be written
 * @param offset Offset into the configuration space in bytes
 * @param size Size in bytes
 * @param *buffer Input buffer
 * @return Status code
*/
STATUS IoWriteConfigSpace(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void *buffer);


/**
 * @brief Get resources associated with given device
 * @param *dev Device to get the resources of
 * @param **res Output buffer for resource list. Allocated by the callee.
 * @param *count Output number of resource entries
 * @return Status code
*/
STATUS IoGetDeviceResources(struct IoDeviceObject *dev, struct IoDeviceResource **res, uint32_t *count);


/**
 * @brief Get device location information
 * @param *dev Device to find the location of
 * @param *type Output for bus type
 * @param *location Output for bus-specific location information
 * @return Status code
*/
STATUS IoGetDeviceLocation(struct IoDeviceObject *dev, enum IoBusType *type, union IoBusId *location);

/**
 * @brief Get top device in the stack containing given device
 * @param *dev Any device in the stack
 * @return Topmost stack device
 */
struct IoDeviceObject* IoGetDeviceStackTop(struct IoDeviceObject *dev);

/**
 * @brief Get device associated with given device file
 * @param *node VFS node of type \a IO_VFS_DEVICE
 * @param **dev Output device object
 * @return Status code
 */
STATUS IoGetDeviceForFile(struct IoVfsNode *node, struct IoDeviceObject **dev);

END_EXPORT_API

/**
 * @brief Notify device enumerator thread that a new device has been created
 * @param *node Device node
 * @return Status code
*/
INTERNAL STATUS IoNotifyDeviceEnumerator(struct IoDeviceNode *node);

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

#endif