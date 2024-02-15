//This header file is generated automatically
#ifndef EXPORTED___API__IO_DEV_DEV_H_
#define EXPORTED___API__IO_DEV_DEV_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "ex/kdrv.h"
#include "rp.h"
#include "typedefs.h"
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

struct IoSubDeviceObject;

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

/**
 * @brief Create device subobject from driver
 * @param *driver Driver object pointer
 * @param privateSize Size of \a (*device)->privateData to allocate
 * @param flags Device flags
 * @param **device Output subdevice object pointer. The memory is allocated by this function.
 * @return Status code
 * @attention Created device must be attached to the stack using \a IoAttachSubDevice().
*/
extern STATUS IoCreateSubDevice(
    struct ExDriverObject *driver, 
    IoDeviceFlags flags, 
    struct IoSubDeviceObject **device);

/**
 * @brief Attach subdevice to the top of the device stack
 * @param *attachee Subdevice object to be attached
 * @param *destination Any subdevice object in given device stack
 * @return Pointer to the previous top device stack object or NULL on failure
*/
extern struct IoSubDeviceObject* IoAttachSubDevice(
    struct IoSubDeviceObject *attachee, 
    struct IoSubDeviceObject *destination);

/**
 * @brief Register new device to be used by the system
 * @param *baseDevice Base (bus driver) subdevice object
 * @param *deviceId Device ID string
 * @return Status code
 * @warning This function should be called only by (or on behalf of) base (bus) subdevice object
*/
extern STATUS IoRegisterDevice(struct IoSubDeviceObject *baseDevice, char *deviceId);

/**
 * @brief Initialize registered device - notify the device manager
 * @param *dev Registered device object
 * @return Status code
*/
extern STATUS IoInitializeDevice(struct IoDeviceObject *dev);

/**
 * @brief Build device stack (load drivers and add subdevices) for previously registered device
 * @param *device Device object
 * @return Status code
 * @warning Device must be registered with IoRegisterDevice() first
*/
extern STATUS IoBuildDeviceStack(struct IoDeviceObject *device);

/**
 * @brief Send Request Packet to a given subdevice or top of a given device stack
 * @param *subDevice Subdevice object
 * @param *device Device stack object. Used to find the device stack top when subDevice is NULL
 * @param *rp RP to be sent
 * @return Status code
*/
extern STATUS IoSendRp(struct IoSubDeviceObject *subDevice, struct IoDeviceObject *device, struct IoDriverRp *rp);

/**
 * @brief Send/pass Request Packet down the stack
 * @param *rp RP to be sent down
 * @return Status code
 * @attention This function fails if there are no more subdevices in the stack
*/
extern STATUS IoSendRpDown(struct IoDriverRp *rp);

/**
 * @brief Set displayed (friendly) name of a device
 * @param *device Subdevice object (any for given main device)
 * @param *name Name to assign
 * @return Status code
*/
extern STATUS IoSetDeviceDisplayedName(struct IoSubDeviceObject *device, char *name);

/**
 * @brief Add new device ID to compatible device ID list
 * @param *dev Target device object
 * @param *id Device ID to be added. If NULL, nothing will be added.
 * @return Status code
*/
extern STATUS IoUpdateCompatibleDeviceIdList(struct IoDeviceObject *dev, char *id);


#ifdef __cplusplus
}
#endif

#endif