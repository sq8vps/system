/*
 * pci.h
 *
 *  Created on: 26.12.2020
 *      Author: Piotr
 */

#ifndef LOADER_PCI_H_
#define LOADER_PCI_H_

#include <stdint.h>
#include "common.h"
#include "port.h"
#include "defines.h"

#ifdef __DEBUG
#include "disp.h"
#endif

#define PCI_STD_VENID 0
#define PCI_STD_DEVID 2
#define PCI_STD_CMD 4
#define PCI_STD_STATUS 6
#define PCI_STD_REV 8
#define PCI_STD_PROGIF 9
#define PCI_STD_SUBCLASS 10
#define PCI_STD_CLASS 11
#define PCI_STD_CACHESZ 12
#define PCI_STD_LATTIM 13
#define PCI_STD_HEADERTYPE 14
#define PCI_STD_BIST 15
#define PCI_STD_BAR0 16
#define PCI_STD_BAR1 20
#define PCI_STD_BAR2 24
#define PCI_STD_BAR3 28
#define PCI_STD_BAR4 32
#define PCI_STD_BAR5 36
#define PCI_IDE_TIMING_PRIMARY 64
#define PCI_IDE_TIMING_SECONDARY 66


#define PCI_BRIDGE_BAR0 16
#define PCI_BRIDGE_BAR1 20
#define PCI_BRIDGE_SECBUS 25

#define PCI_HEADER_MF_BIT 0x80

#define PCI_VENDOR_INVALID 0xFFFF

//vendor ID is at offset 0, device id is at offset 2, both are 16 bit long. So they both can be read simultaneosly as a 32-bit dword
//AMD devices
#define PCI_DEV_AMD_645_IDE 0x05711106
#define PCI_DEV_AMD_756_IDE 0x74091022
#define PCI_DEV_AMD_768_IDE 0x74411022
#define PCI_DEV_AMD_766_IDE 0x74111022
#define PCI_DEV_AMD_8111_IDE 0x74691022

//Intel devices
#define PCI_DEV_PIIX_IDE 0x12308086
#define PCI_DEV_PIIX3_IDE 0x70108086
#define PCI_DEV_PIIX4_IDE 0x71118086

#define PCI_HEADER_INVALID 0xFF



#define PCI_CLASS_INVALID 0xFF
#define PCI_SUBCLASS_INVALID 0xFF



#define PCI_CLASS_BRIDGE 0x06
#define PCI_SUBCLASS_PCI_PCI 0x04
#define PCI_CLASS_STORAGE 0x01
#define PCI_SUBCLASS_IDE 0x01
#define PCI_SUBCLASS_ATA 0x05
#define PCI_SUBCLASS_SATA 0x06


typedef struct
{
	uint8_t bus; /**< Bus number **/
	uint8_t dev; /**< Device number **/
	uint8_t func; /**< Function number **/
} Pci_s_t;

/**
 * \brief Writes data to selected BARx register
 * \param pci PCI structure
 * \param bar BARx register number (x: 0-5)
 * \param data Data to be written
 */
void pci_setBAR(Pci_s_t pci, uint8_t bar, uint32_t data);

/**
 * \brief Reads vendor ID
 * \param pci PCI structure
 * \return Vendor ID or 0xFFFF if device non-existent
 */
uint16_t pci_getVendor(Pci_s_t pci);

/**
 * \brief Scans all buses
 * \attention This is the only PCI enumeration function that can be called from the outside.
 */
void pci_scanAll(void);


/**
 * \brief Reads programming inteface data
 * \param pci PCI structure
 * \return Programming interface data or PCI_PROGIF_INVALID if device non-existent
 */
uint8_t pci_getProgIf(Pci_s_t pci);

/**
 * \brief Writes to IDE command register
 * \param pci PCI structure
 * \param data Data to be written
 */
void pci_setIdeCommand(Pci_s_t pci, uint16_t data);

/**
 * \brief Sets/resets IDE Decode Enable bit
 * \param pci PCI structure
 * \param primsec 0 - primay channel, 1 - secondary channel
 * \param state 0 - reset, 1 - set
 */
void pci_setIdeDecode(Pci_s_t pci, uint8_t primsec, uint16_t state);

/**
 * \brief Sets callback function for ATA controller enumeration
 * \param *f Callback function pointer, argument 0 is a PCI structure, argument 1 is a 32-bit dword containing vendor ID and device ID
 */
void pci_setAtaEnumCallback(void (*f)(Pci_s_t, uint32_t));

/**
 * \brief Sets callback function for SATA AHCI controller enumeration
 * \param *f Callback function pointer, argument 0 is a PCI structure, argument 1 is a 32-bit dword containing vendor ID and device ID
 */
void pci_setSataEnumCallback(void (*f)(Pci_s_t, uint32_t));

#endif /* LOADER_PCI_H_ */
