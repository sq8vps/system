#ifndef IDE_ATA_H_
#define IDE_ATA_H_

#include <stdint.h>
#include "defines.h"
#include "device.h"

/**
 * @brief Perform detection of drives connected to a given controller
 * @param *ide Controller structure pointer
 * @return Status code
*/
STATUS IdeDetectAllDrives(struct IdeControllerData *ide);

/**
 * @brief Create and register new drive device
 * @param *drive Preallocated drive structure pointer
 * @param *driver Appropriate driver object
 * @return Status code
*/
STATUS IdeCreateDriveDevice(struct IdeDriveData *drive, struct ExDriverObject *driver);

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

#endif