#ifndef IDE_DEVICE_H_
#define IDE_DEVICE_H_

#include "defines.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "ke/core/mutex.h"
#include <stdbool.h>

struct IdeDriverData
{
    uint32_t controllerCount;
};

/**
 * @brief Driver serial number length defined by ATA
*/
#define ATA_DRIVE_SERIAL_NUMBER_SIZE 20

/**
 * @brief Driver model number length defined by ATA
*/
#define ATA_DRIVE_MODEL_NUMBER_SIZE 40

/**
 * @brief Physical Region Descriptor entry
*/
struct IdePrdEntry
{
    uint32_t address;
    uint16_t size;
    uint16_t eot;
} PACKED;

/**
 * @brief Internal PRD table structure
 * 
 * Contains pointer to the PRD table and its physical address
*/
struct IdePrdTable
{
    struct IdePrdEntry *table;
    uint32_t physical;
};

struct IdeControllerData;
struct IdeDeviceData;

/**
 * @brief IDE drive structure
*/
struct IdeDriveData
{
    char serial[ATA_DRIVE_SERIAL_NUMBER_SIZE + 1]; /**< Serial number in ATA standard */
    char model[ATA_DRIVE_MODEL_NUMBER_SIZE + 1]; /**< Model number in ATA standard */
    uint8_t present : 1; /**< Is drive present? If not, this structure is invalid */
    uint8_t usable : 1; /**< Is usable by this driver? */
    uint8_t lba48 : 1; /**< Does the drive support LBA48 addressing? If not, only LBA28 is available */
    uint64_t sectors; /**< Number of sectors */
    uint32_t sectorSize; /**< Logical sector size */

    uint8_t channel; /**< Channel number that this drive is connected to */
    uint8_t drive; /**< Slot number that this drive is connected to */
    struct IdeControllerData *controller; /**< Reference to controller data */
};

#define PCI_IDE_CHANNEL_PRIMARY 0x00
#define PCI_IDE_CHANNEL_SECONDARY 0x01

#define PCI_IDE_SLOT_MASTER 0x00
#define PCI_IDE_SLOT_SLAVE 0x01

#define IDE_BMR_COMMAND_START 0x01
#define IDE_BMR_COMMAND_RW_CONTROL 0x08

#define IDE_BMR_STATUS_ACTIVE 0x01
#define IDE_BMR_STATUS_ERROR 0x02
#define IDE_BMR_STATUS_INTERRUPT 0x04

#define IDE_PRD_MAX_SIZE 65536

/**
 * @brief IDE controller structure
*/
struct IdeControllerData
{
    struct IoDeviceObject *enumerator; /**< Reference to enumerating device */
    bool initialized; /**< Is controller fully initialized? */
    bool busMaster; /**< Does the controller support Bus Mastering? If not, it is not usable here */
    struct
    {
        //I/O ports
        uint16_t cmdPort;
        uint16_t controlPort;
        uint16_t masterPort;
        //last selected slot/device
        uint8_t lastSelectedSlot : 1;

        //buffer and PRD table
        struct IdePrdTable prdt;
        struct IoMemoryDescriptor *buffer;

        struct IoRpQueue *queue;
        struct IoRp *rp;
        
        struct
        {
            uint8_t write : 1;
            uint8_t busy : 1;
            uint8_t slot;
            uint64_t lba;
            uint64_t size;
            uint64_t remaining;
            uint8_t retries;
            struct IoMemoryDescriptor memory;
        } operation;

        KeSpinlock lock;

        struct IdeDeviceData *drive[2];
    } channel[2];
};

struct IdeDeviceData
{
    uint8_t isController : 1;
    union
    {
        struct IdeControllerData controller;
        struct IdeDriveData drive;
    };
};

/**
 * @brief Process request packet - callback function for IO manager
*/
void IdeProcessRequest(struct IoRp *rp);

/**
 * @brief Create drive device objects for all present drives
 * @param *info Controller data structure
 * @param *dev Enumerator device object
 * @param *driver IDE driver object
 * @return Status code
*/
STATUS IdeCreateAllDriveDevices(struct IdeControllerData *info, struct IoDeviceObject *dev, struct ExDriverObject *driver);

/**
 * @brief General IDE interrupt service routine
 * @param *context ISR context, a IdeControllerData structure
*/
STATUS IdeIsr(void *context);

/**
 * @brief Verify request and start read/write to drive
 * 
 * Verify request parameters (alignment, size, memory buffer, etc.) and start requested operation.
 * The function returns immediately. The end of the operation is reported through a completion callback
 * provided in the Request Packet.
 * 
 * @param *info Drive data structure
 * @param write True if writing, false if reading
 * @param offset Offset to start from in bytes
 * @param size Size in bytes
 * @param *buffer A memory descriptor list for the operation
 * @return Status code
 * @attention This function returns fast. The operation is always asynchronous.
*/
STATUS IdeReadWrite(struct IdeDriveData *info, bool write, uint64_t offset, uint64_t size, struct IoMemoryDescriptor *buffer);

STATUS IdeGetDeviceId(struct IoRp *rp);

#endif