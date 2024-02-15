#ifndef KERNEL_DEV_H_
#define KERNEL_DEV_H_

#include <stdint.h>
#include "defines.h"
#include "ex/kdrv.h"
#include "rp.h"
#include "typedefs.h"

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
};

EXPORT
struct IoSubDeviceObject;

EXPORT
struct IoDeviceObject
{
    char *deviceId;
    #define IO_MAX_COMPATIBLE_DEVICE_IDS 8
    char *compatibleIds[IO_MAX_COMPATIBLE_DEVICE_IDS];
    char *name;
    enum IoDeviceType type;
    IoDeviceFlags flags;
    struct IoSubDeviceObject *baseDevice;
    struct IoDeviceObject *parent;
    struct IoDeviceObject *child;
    struct IoDeviceObject *next, *previous;
};

EXPORT
struct IoSubDeviceObject
{
    struct ExDriverObject *driverObject;
    struct IoSubDeviceObject *nextDevice; //next device created by the same driver
    struct IoSubDeviceObject *attachedTo;
    struct IoSubDeviceObject *attachedDevice;
    struct IoDriverRp *currentRp;
    uint32_t referenceCount;
    IoDeviceFlags flags;
    void *privateData;
    struct IoDeviceObject *mainDeviceObject;
};

EXPORT
/**
 * @brief Create device subobject from driver
 * @param *driver Driver object pointer
 * @param privateSize Size of \a (*device)->privateData to allocate
 * @param flags Device flags
 * @param **device Output subdevice object pointer. The memory is allocated by this function.
 * @return Status code
 * @attention Created device must be attached to the stack using \a IoAttachSubDevice().
*/
EXTERN STATUS IoCreateSubDevice(
    struct ExDriverObject *driver, 
    IoDeviceFlags flags, 
    struct IoSubDeviceObject **device);

EXPORT
/**
 * @brief Attach subdevice to the top of the device stack
 * @param *attachee Subdevice object to be attached
 * @param *destination Any subdevice object in given device stack
 * @return Pointer to the previous top device stack object or NULL on failure
*/
EXTERN struct IoSubDeviceObject* IoAttachSubDevice(
    struct IoSubDeviceObject *attachee, 
    struct IoSubDeviceObject *destination);

EXPORT
/**
 * @brief Register new device to be used by the system
 * @param *baseDevice Base (bus driver) subdevice object
 * @param *deviceId Device ID string
 * @return Status code
 * @warning This function should be called only by (or on behalf of) base (bus) subdevice object
*/
EXTERN STATUS IoRegisterDevice(struct IoSubDeviceObject *baseDevice, char *deviceId);

EXPORT
/**
 * @brief Initialize registered device - notify the device manager
 * @param *dev Registered device object
 * @return Status code
*/
EXTERN STATUS IoInitializeDevice(struct IoDeviceObject *dev);

EXPORT
/**
 * @brief Build device stack (load drivers and add subdevices) for previously registered device
 * @param *device Device object
 * @return Status code
 * @warning Device must be registered with IoRegisterDevice() first
*/
EXTERN STATUS IoBuildDeviceStack(struct IoDeviceObject *device);

/**
 * @brief Initialize device manager and create root device
 * @param *rootDeviceId Root device ID string (to find appropriate driver)
 * @return Status code
*/
INTERNAL STATUS IoInitDeviceManager(char *rootDeviceId);

EXPORT
/**
 * @brief Send Request Packet to a given subdevice or top of a given device stack
 * @param *subDevice Subdevice object
 * @param *device Device stack object. Used to find the device stack top when subDevice is NULL
 * @param *rp RP to be sent
 * @return Status code
*/
EXTERN STATUS IoSendRp(struct IoSubDeviceObject *subDevice, struct IoDeviceObject *device, struct IoDriverRp *rp);

EXPORT
/**
 * @brief Send/pass Request Packet down the stack
 * @param *rp RP to be sent down
 * @return Status code
 * @attention This function fails if there are no more subdevices in the stack
*/
EXTERN STATUS IoSendRpDown(struct IoDriverRp *rp);

EXPORT
/**
 * @brief Find device stack top subdevice
 * @param *dev Device object to find the top subdevice of
 * @return Top subdevice object
*/
EXTERN struct IoSubDeviceObject* IoGetDeviceStackStop(struct IoDeviceObject *dev);

EXPORT
/**
 * @brief Set displayed (friendly) name of a device
 * @param *device Subdevice object (any for given main device)
 * @param *name Name to assign
 * @return Status code
*/
EXTERN STATUS IoSetDeviceDisplayedName(struct IoSubDeviceObject *device, char *name);

EXPORT
/**
 * @brief Add new device ID to compatible device ID list
 * @param *dev Target device object
 * @param *id Device ID to be added. If NULL, nothing will be added.
 * @return Status code
*/
EXTERN STATUS IoUpdateCompatibleDeviceIdList(struct IoDeviceObject *dev, char *id);

#endif