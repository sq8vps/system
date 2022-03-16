/*
 * ata.h
 *
 *  Created on: 27.12.2020
 *      Author: Piotr
 */

#ifndef LOADER_ATA_H_
#define LOADER_ATA_H_

#define ATA_MAX_CONTROLLERS 4

#include <stdint.h>
#include "pci.h"
#include "common.h"
#include "defines.h"
#include "port.h"
#include "disk.h"

#ifdef __DEBUG
#include "disp.h"
#endif




#define ATA_BMR_PORT_BASE 0xD000
#define ATA_BMR_PRIMARY 0
#define ATA_BMR_SECONDARY 8
#define ATA_BMR_CMD 0
#define ATA_BMR_STA 2
#define ATA_BMR_SIZE 16
#define ATA_BMR_CMD_RWCON_BIT 8
#define ATA_BMR_CMD_SSBM_BIT 1
#define ATA_BMR_STA_ACT_BIT 1
#define ATA_BMR_STA_ERR_BIT 2
#define ATA_BMR_STA_INT_BIT 4

//base I/O ports for PIIX/PIIX3/PIIX4 ATA command blocks
#define ATA_PIIX_CMD_PORT_PRI_BASE 0x1f0
#define ATA_PIIX_CMD_PORT_SEC_BASE 0x170

#define ATA_CMD_PORT_DATA 0
#define ATA_CMD_PORT_FEA_ERR 1
#define ATA_CMD_PORT_SECCNT 2
#define ATA_CMD_PORT_LBA_LOW 3
#define ATA_CMD_PORT_LBA_MID 4
#define ATA_CMD_PORT_LBA_HIGH 5
#define ATA_CMD_PORT_DEV 6
#define ATA_CMD_PORT_CMD_STA 7


#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READDMA 0x25
#define ATA_CMD_WRITEDMA 0x35

#define ATA_CMD_DEV_MSTR_SLV_BIT 16
#define ATA_CMD_DEV_LBA_BIT 64
#define ATA_CMD_STA_BSY_BIT 128
#define ATA_CMD_STA_ERR_BIT 1
#define ATA_CMD_STA_DRQ_BIT 8


//TODO: enum

//generic controller types
#define ATA_IDE_UNKNOWN 0
#define ATA_IDE_PIIX 1 //for Intel PIIX series
#define ATA_IDE_AMD_XXX 2 //for AMD-nnn and AMD-nnnn series


//TODO: bit fields
typedef struct
{
	Pci_s_t pci; //PCI address of the controller
	uint8_t present; //0 - controller not present, 1 - controller present
	uint8_t usable; //0 - unusable (unknown or incompatible), 1 - usable
	uint16_t type; //generic controller type
	struct //disk structure
	{
		uint8_t present; //disk is present and accessible
		uint16_t sectorSize; //logical sector size in bytes
		uint64_t sectorCount; //logical sector count
	} disk[2][2]; //structures for disks (primary master, primary slave, secondary master, secondary slave)
	uint16_t bmrPort; //I/O port address for BMR table
	uint16_t cmdPort[2]; //I/O ports for ATA command/data block (0 - primary, 1 - secondary)
} AtaController_s_t; //ATA controller structure




/**
 * \brief Adds ATA controller to the list
 * \param pci Pci_s_t structure of the controller
 * \param id Vendor ID in lower word, Device ID in higher word
 * \warning This function doesn't check if it is a ata controller.
 */
void ata_registerController(Pci_s_t pci, uint32_t id);

/**
 * \brief Enables (sets up) ATA controllers
 * \attention Controllers must be registered first using ata_registerController()
 */
void ata_enableControllers(void);

/**
 * \brief Initializes ATA tables, register, buffers etc.
 */
void ata_init(void);


/**
 * \brief Reads/writes sectors from/to IDE drive using DMA
 * \param ata ATA controller structure
 * \param rw 0 - read from drive, 1 - write to drive
 * \param chan Primary (0) or Secondary (1)
 * \param dev Master (0) or Slave (1)
 * \param *buf Destination/source buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param sec Sector count (higher than 0)
 * \return ATA_OK if successful, otherwise destination buffer/destination sectors must be invalidated
 */
error_t ata_IDEreadWrite(AtaController_s_t ata, uint8_t rw, uint8_t chan, uint8_t dev, uint8_t *buf, uint64_t lba, uint16_t sec);


/**
 * \brief Sets callback function for adding new disk entry to the disk table
 * \param void Callback function to be called
 */
void ata_setAddDiskCallback(void (*callback)(void*, uint8_t));

#endif /* LOADER_ATA_H_ */
