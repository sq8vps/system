//This header file is generated automatically
#ifndef EXPORTED___API__IO_DEV_DEV_H_
#define EXPORTED___API__IO_DEV_DEV_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "bus.h"
#include "ob/ob.h"

#define IO_MAX_COMPATIBLE_DEVICE_IDS 8


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
#define IO_DEVICE_FLAG_NO_AUTOMOUNT 0x10000000

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
    IoDeviceFlags flags, 
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


#ifdef __cplusplus
}
#endif

#endif