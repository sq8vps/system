#ifndef IDE_ATA_H_
#define IDE_ATA_H_

#include <stdint.h>
#include "defines.h"
#include "device.h"

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_SRV 0x10
#define ATA_STATUS_DF 0x20
#define ATA_STATUS_RDY 0x40
#define ATA_STATUS_BSY 0x80

#define ATA_ERROR_AMNF 0x01
#define ATA_ERROR_TKZNF 0x01
#define ATA_ERROR_ABRT 0x04
#define ATA_ERROR_MCR 0x08
#define ATA_ERROR_IDNF 0x10
#define ATA_ERROR_MC 0x20
#define ATA_ERROR_UNC 0x40
#define ATA_ERROR_BBK 0x80

/**
 * @brief Select device to operate on
 * @param *ide IDE controller structure
 * @param channel Channel (0 or 1)
 * @param slot Slot (::PCI_IDE_SLOT_MASTER or ::PCI_IDE_SLOT_SLAVE)
 */
void AtaSelectDrive(struct IdeControllerData *ide, uint8_t channel, uint8_t slot);

/**
 * @brief Perform detection of drives connected to a given controller
 * @param *ide Controller structure pointer
 * @return Status code
*/
STATUS IdeDetectAllDrives(struct IdeControllerData *ide);

/**
 * @brief Create and register new drive device
 * @param *drive IDE device data for drive pointer
 * @param *enumerator Enumerating device
 * @param *driver Appropriate driver object
 * @return Status code
*/
STATUS IdeCreateDriveDevice(struct IdeDeviceData *drive, struct IoDeviceObject *enumerator, struct ExDriverObject *driver);

/**
 * @brief Write parameters for read/write operation using LBA28
 * @param *ide Controller data structure
 * @param channel Channel number
 * @param slot Drive number
 * @param lba Starting LBA
 * @param sectors Sector count
*/
void IdeWriteLba28Parameters(struct IdeControllerData *ide, uint8_t channel, uint8_t slot, uint32_t lba, uint8_t sectors);

/**
 * @brief Write parameters for read/write operation using LBA48
 * @param *ide Controller data structure
 * @param channel Channel number
 * @param slot Drive number
 * @param lba Starting LBA
 * @param sectors Sector count
*/
void IdeWriteLba48Parameters(struct IdeControllerData *ide, uint8_t channel, uint8_t slot, uint64_t lba, uint16_t sectors);

/**
 * @brief Issue read/write ATA command to drive
 * @param *info Controller data structure
 * @param channel Channel number
 * @param slot Drive number
 * @param write True if writing, false if reading
 * @param lba48 True if LBA48 is used, false if LBA28 is used
*/
void IdeStartTransfer(struct IdeControllerData *info, uint8_t channel, uint8_t slot, bool write, bool lba48);

/**
 * @brief Get ATA channel status
 * @param *info Controller data structure
 * @param channel Channel number
 * @return Channel status
 */
uint8_t AtaGetChannelStatus(struct IdeControllerData *info, uint8_t channel);

#endif