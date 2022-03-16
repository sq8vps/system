#include "ata.h"


#define ATA_MAX_PRD_ENTRIES 1024

#define ATA_PRD_TABLE_PRIMARY 0
#define ATA_PRD_TABLE_SECONDARY 1

#define ATA_BMR_CMD_START_STOP_BIT 0b001
#define ATA_BMR_CMD_READ_WRITE_BIT 0b1000



typedef struct
{
	uint8_t cmd; //command register
	uint8_t status; //status register
	uint64_t *addr; //PRD Table address refister
} AtaBMR_s_t; //BMR table entry structre

typedef struct
{
	uint16_t size; //byte count register
	uint8_t *addr; //destination memory address register
} AtaPRD_s_t; //PRD entry structure


AtaController_s_t ata_controllerList[ATA_MAX_CONTROLLERS]; //ATA controllers table

uint64_t ata_PRDtable[2][ATA_MAX_PRD_ENTRIES] __attribute__ ((aligned (8))) = {0}; //PRD tables (two, for primary and secondary).
																				   //This table must be 4-byte aligned, but keep it 8-byte aligned as it's more standard
uint16_t ata_PRDtableEntries[2] = {0}; //number of entries in each PRD Table


void (*ata_addDiskCallback)(void*, uint8_t) = (void*)0;

void ata_writeBMR(AtaController_s_t ata, AtaBMR_s_t pri, AtaBMR_s_t sec);

/**
 * \brief Primitive sleep function for ATA purposes.
 */
volatile void ata_sleep()
{
	uint64_t to = 0;
	while(to < 0xFFFF) //this delay was chosen rather experimentally
	{
		to++;
	}
}

/**
 * \brief Adds ATA controller to the list
 * \param pci Pci_s_t structure of the controller
 * \param id Vendor ID in lower word, Device ID in higher word
 * \warning This function doesn't check if it is a ata controller.
 */
void ata_registerController(Pci_s_t pci, uint32_t id)
{
	uint8_t i = 0;
	for(; i < ATA_MAX_CONTROLLERS; i++) //check the controller list in case the controller was already listed
	{
		if(ata_controllerList[i].present == 0) break; //this slot is free
		if((ata_controllerList[i].pci.bus == pci.bus) && (ata_controllerList[i].pci.dev == pci.dev) && (ata_controllerList[i].pci.func == pci.func)) return; //if this controller was already listed, skip it
	}
	if(i == (ATA_MAX_CONTROLLERS)) return; //check if there is enough room for the next entry
	//if so, add it to the list
	ata_controllerList[i].present = 1;
	ata_controllerList[i].pci = pci;
	switch(id) //match known IDE controller models with more generic controller type. All models in a group are handled the same way.
	{
		case PCI_DEV_AMD_645_IDE:
		case PCI_DEV_AMD_756_IDE:
		case PCI_DEV_AMD_766_IDE:
		case PCI_DEV_AMD_768_IDE:
		case PCI_DEV_AMD_8111_IDE:
			ata_controllerList[i].type = ATA_IDE_AMD_XXX;

			break;
		case PCI_DEV_PIIX_IDE:
		case PCI_DEV_PIIX3_IDE:
		case PCI_DEV_PIIX4_IDE:
			ata_controllerList[i].type = ATA_IDE_PIIX;
			ata_controllerList[i].cmdPort[0] = ATA_PIIX_CMD_PORT_PRI_BASE; //set base I/O ports
			ata_controllerList[i].cmdPort[1] = ATA_PIIX_CMD_PORT_SEC_BASE;
			ata_controllerList[i].bmrPort = ATA_BMR_PORT_BASE + (i * ATA_BMR_SIZE);
			break;
		default:
			ata_controllerList[i].type = ATA_IDE_UNKNOWN;
			break;
	}
}

/**
 * \brief Detects connected drives
 * \attention Controllers must be enabled first using ata_enableControllers
 */
void ata_detectDrives(void)
{
	for(uint8_t i = 0; i < ATA_MAX_CONTROLLERS; i++)
	{
		if(ata_controllerList[i].usable == 0) continue; //skip unusable controllers

		for(uint8_t l = 0; l < 2; l++) //loop for primary and secondary
		{
			uint16_t port = ata_controllerList[i].cmdPort[l]; //set base port number

			for(uint8_t j = 0; j < 2; j++) //loop for primary master and slave
			{
				port_writeByte(port + ATA_CMD_PORT_DEV, j ? ATA_CMD_DEV_MSTR_SLV_BIT : 0); //check primary master or slave
				ata_sleep();
				port_writeByte(port + ATA_CMD_PORT_CMD_STA, ATA_CMD_IDENTIFY); //issue command
				ata_sleep();
				if(port_readByte(port + ATA_CMD_PORT_CMD_STA) == 0) continue; //if status register is 0, this drive doesnt exist
				uint8_t err = 0;
				while(1)
				{
					uint8_t status = port_readByte(port + ATA_CMD_PORT_CMD_STA);
					if(status & ATA_CMD_STA_ERR_BIT) //check if error bit set
					{
						err = 1; //if so, the drive is not present
						break;
					}
					if(!(status & ATA_CMD_STA_BSY_BIT) && (status & ATA_CMD_STA_DRQ_BIT))
						break; //if drive is not busy and is ready for data transfer, everything is ok
				}
				if(err) continue;

				for(uint16_t k = 0; k < 63; k++)
					port_readWord(port); //drop returned data

				err = ((port_readWord(port) & 0b111) == 0); //check if mutliword DMA transfer is supported (at word 63)

				for(uint16_t k = 64; k < 100; k++)
					port_readWord(port); //drop returned data

				uint64_t sectors = (uint64_t)port_readWord(port) | ((uint64_t)port_readWord(port) << 16) | ((uint64_t)port_readWord(port) << 32) | ((uint64_t)port_readWord(port) << 48); //number of logical sectors at words 100:103
				if(sectors == 0) err = 1;

				port_readWord(port); //dummy read at word 104
				port_readWord(port); //dummy read at word 105

				uint16_t sectorSize = 0;

				//ATA command set is VERY poorly readable, but to determine the logical sector size:
				//Check if bit 15 equals 0, bit 14 equals 1 and bit 12 equals 1. Bit 12 is labeled as "logical sector longer than 256 words" which doesn't really tell anything.
				//In fact this bit is a copy of "logical sector size supported" bit. This means that if bit 15->0, 14->1 and 12->1, the logical sector size field (words 117:118) is available,
				//so we can just read it to determine the logical sector size.
				//If these bits are not set as expected, the logical sector size field is unavailable and we must assume the logical sector to be 512 bytes long.
				if((port_readWord(port) & 0b1101000000000000) == 0b0101000000000000)
				{
					for(uint16_t k = 107; k < 117; k++)
						port_readWord(port); //drop returned data

					//then the logical sector size field (words 117:118) is available
					sectorSize = (uint32_t)port_readWord(port) | ((uint32_t)port_readWord(port) << 16); //logical sector size at words 117:118
				}
				else //if these bit are not as expected, the sector size equals 512
				{
					sectorSize = 512;
					for(uint16_t k = 107; k < 118; k++)
						port_readWord(port); //drop returned data
				}

				if(sectorSize == 0 || (sectorSize > 0xFFFF)) err = 1; //sector size of 0 is an error. Also we don't want to work with sectors larger than 65535 bytes

				for(uint16_t k = 119; k < 256; k++)
					port_readWord(port); //drop returned data


				if(err) continue;


				if(ata_addDiskCallback == (void*)0) return;

#if __DEBUG > 1
				printf("\nDrive at ATA controller %d %s %s, ", (int)i, l ? "secondary" : "primary", j ? "slave" : "master");
				printf("%d sectors, sector size %d\n", (int)sectors, (int)sectorSize);
#endif

				ata_controllerList[i].disk[l][j].present = 1; //add disk to controller structure
				ata_controllerList[i].disk[l][j].sectorSize = (uint16_t)sectorSize;
				ata_controllerList[i].disk[l][j].sectorCount = sectors;

				(*ata_addDiskCallback)((void*)&(ata_controllerList[i]), (l ? DISK_TABLE_ATTR_CHAN_BIT : 0) | (j ? DISK_TABLE_ATTR_PRIMSEC_BIT : 0)); //add disk to the list




			}
		}
	}
}

/**
 * \brief Enables (sets up) ATA controllers and detects connected drives
 * \attention Controllers must be registered first using ata_registerController()
 */
void ata_enableControllers(void)
{
	for(uint8_t i = 0; i < ATA_MAX_CONTROLLERS; i++)
	{
		if(ata_controllerList[i].present == 0) break; //if this controller is not present, next entries are also not present
		if(ata_controllerList[i].type == ATA_IDE_UNKNOWN) continue; //we also don't know how to handle unknown controllers

		if(pci_getProgIf(ata_controllerList[i].pci) != 0x80) continue; //check if this controller supports bus master operations
		ata_controllerList[i].usable = 1; //if so, it's usable

		pci_setBAR(ata_controllerList[i].pci, 4, (uint32_t)(ata_controllerList[i].bmrPort & 0xFFF0));
		AtaBMR_s_t pri = {0, 0, (uint64_t*)&(ata_PRDtable[0])};
		AtaBMR_s_t sec = {0, 0, (uint64_t*)&(ata_PRDtable[1])};
		ata_writeBMR(ata_controllerList[i], pri, sec);

		pci_setIdeCommand(ata_controllerList[i].pci, 0b101); //enable bus master and i/o space

#if __DEBUG > 1
		printf("\nATA Controller %d at bus %d, dev %d, func %d, type ", (int)i, (int)ata_controllerList[i].pci.bus, (int)ata_controllerList[i].pci.dev, (int)ata_controllerList[i].pci.func);
#endif

		if(ata_controllerList[i].type == ATA_IDE_PIIX)
		{
			pci_setIdeDecode(ata_controllerList[i].pci, 0, 1); //enable IDE decode for primary channel
			pci_setIdeDecode(ata_controllerList[i].pci, 1, 1); //enable IDE decode for secondary channel
#if __DEBUG > 1
			printf("PIIX/PIIX3/PIIX4");
#endif
		}
		else if(ata_controllerList[i].type == ATA_IDE_AMD_XXX)
		{
#if __DEBUG > 1
			printf("AMD-XXX/AMD-XXXX");
#endif
		}
	}
	ata_detectDrives();
}


/**
 * \brief Writes BMR registers to BMR I/O ports.
 * \param ata ATA controller structure
 * \param pri BMR register for primary channel.
 * \param sec BMR register for secondary channel
 */
void ata_writeBMR(AtaController_s_t ata, AtaBMR_s_t pri, AtaBMR_s_t sec)
{
	port_writeByte(ata.bmrPort, pri.cmd);
	port_writeByte(ata.bmrPort + 2, 0b00000111); //clear status bits
	port_writeDword(ata.bmrPort + 4, (uint32_t)pri.addr & 0xFFFFFFFC); //bits 0:1 are reserved
	port_writeByte(ata.bmrPort + 8, sec.cmd);
	port_writeByte(ata.bmrPort + 10, 0b00000111); //clear status bits
	port_writeDword(ata.bmrPort + 12, (uint32_t)sec.addr & 0xFFFFFFFC); //bits 0:1 are reserved
}

/**
 * \brief Writes BMR command to I/O port
 * \param ata ATA controller structure
 * \param chan Primary (0) or Secondary (1)
 * \param cmd Command to be written
 */
static error_t ata_writeBMRcmd(AtaController_s_t ata, uint8_t chan, uint8_t cmd)
{
	if(chan > 1) return ATA_INCORRECT_VAL;
	port_writeByte(ata.bmrPort + (chan ? ATA_BMR_SECONDARY : ATA_BMR_PRIMARY), cmd);
	return ATA_OK;
}

/**
 * \brief Reads BMR status from I/O port
 * \param ata ATA controller structure
 * \param chan Primary (0) or Secondary (1)
 * \param *sta Status output
 */
static error_t ata_readBMRstatus(AtaController_s_t ata, uint8_t chan, uint8_t *sta)
{
	if(chan > 1) return ATA_INCORRECT_VAL;
	*sta = port_readByte(ata.bmrPort + (chan ? ATA_BMR_SECONDARY : ATA_BMR_PRIMARY) + ATA_BMR_STA);
	return ATA_OK;
}

/**
 * \brief Writes BMR status from I/O port
 * \param ata ATA controller structure
 * \param chan Primary (0) or Secondary (1)
 * \param sta Status to be written
 */
static error_t ata_writeBMRstatus(AtaController_s_t ata, uint8_t chan, uint8_t sta)
{
	if(chan > 1) return ATA_INCORRECT_VAL;
	port_writeByte(ata.bmrPort + (chan ? ATA_BMR_SECONDARY : ATA_BMR_PRIMARY) + ATA_BMR_STA, sta);
	return ATA_OK;
}

/**
 * \brief Adds PRD entry to the PRD Table
 * \param prd PRD entry structure
 * \param prdtable PRD table number (0 - primary, 1 - secondary)
 */
error_t ata_addPRDentry(AtaPRD_s_t prd, uint8_t prdtable)
{
	if(prdtable > 1) return ATA_INCORRECT_VAL;
	if((uint32_t)prd.addr & 1) return ATA_NOT_ALIGNED;
	if(prd.size & 1) return ATA_NOT_ALIGNED;
	if(ata_PRDtableEntries[prdtable] >= ATA_MAX_PRD_ENTRIES) return ATA_COUNT_LIMIT; //no more space

	ata_PRDtable[prdtable][ata_PRDtableEntries[prdtable]] = ((uint32_t)prd.addr & 0xFFFFFFFE) | ((uint64_t)(prd.size & 0xFFFE) << 32) | ((uint64_t)1 << 63); //store address, byte count and add end-of-table bit

	if(ata_PRDtableEntries[prdtable] > 0)
		ata_PRDtable[prdtable][ata_PRDtableEntries[prdtable] - 1] &= 0x7FFFFFFFFFFFFFFF; //if there was a table entry before, clear it's end-of-table bit

	ata_PRDtableEntries[prdtable]++;
	return ATA_OK;
}

/**
 * \brief Clears both PRD Tables
 */
void ata_clearPRDtables(void)
{
	ata_PRDtableEntries[0] = 0;
	ata_PRDtableEntries[1] = 0;
}





/**
 * \brief Reads/writes sectors from/to ATA drive using DMA
 * \param ata ATA controller structure
 * \param rw 0 - read from drive, 1 - write to drive
 * \param chan Primary (0) or Secondary (1)
 * \param dev Master (0) or Slave (1)
 * \param *buf Destination/source buffer, must be word aligned
 * \param lba Starting LBA (48-bit)
 * \param sec Sector count (higher than 0)
 * \return ATA_OK if successful, otherwise destination buffer/destination sectors must be invalidated
 */
error_t ata_IDEreadWrite(AtaController_s_t ata, uint8_t rw, uint8_t chan, uint8_t dev, uint8_t *buf, uint64_t lba, uint16_t sec)
{
	if(chan > 1 || dev > 1) return ATA_INCORRECT_VAL;
	if(rw > 1) return ATA_INCORRECT_VAL;
	if(sec == 0) return ATA_NO_DATA; //nothing to read
	if(sec > ata.disk[chan][dev].sectorCount) return ATA_DISK_TOO_SMALL;
	if((uint32_t)buf & 1) return ATA_NOT_ALIGNED; //check if buffer is word aligned
	if(lba & 0xFFFF000000000000) return ATA_INCORRECT_VAL; //LBA address must fit in the lower 48 bits
	error_t ret = ATA_OK;

	uint32_t bytes = sec * ata.disk[chan][dev].sectorSize; //get number of bytes
	uint16_t blocks = bytes / 65536; //get number of whole 64KiB blocks (for each PRD entry)




	uint32_t freeSpace = 0xFFFFFFFF - (uint32_t)buf; //check available space to avoid wrapping
	if(bytes > freeSpace)
		return ATA_MEMORY_TOO_SMALL;

	ata_clearPRDtables(); //clear PRD tables
	AtaPRD_s_t prd;
	uint16_t i = 0;
	for(i = 0; i < blocks; i++) //loop for 64KiB blocks
	{
		prd.addr = buf + (65536 * i); //next 64KiB block
		prd.size = 0; //according to specs, 0 means 64KiB
		ret = ata_addPRDentry(prd, chan);
		if(ret != ATA_OK) return ret;
	}
	if(bytes % 65536) //if there is some rest
	{
		prd.addr = buf + (65536 * i); //next 64KiB block
		prd.size = bytes % 65536;
		ret = ata_addPRDentry(prd, chan);
		if(ret != ATA_OK) return ret;
	}
	//set up BMRs
	//DMA reads from/writes to the drive and writes to/reads from memory. That's why for reading data from drive we set the RWCON bit and not for writing.
	//additionally clear status bits only for selected channel
	AtaBMR_s_t priBMR = {rw ? 0 : ATA_BMR_CMD_RWCON_BIT, chan ? 0 : (ATA_BMR_STA_ERR_BIT | ATA_BMR_STA_INT_BIT), (uint64_t*)&(ata_PRDtable[0])};
	AtaBMR_s_t secBMR = {rw ? 0 : ATA_BMR_CMD_RWCON_BIT, chan ? (ATA_BMR_STA_ERR_BIT | ATA_BMR_STA_INT_BIT) : 0, (uint64_t*)&(ata_PRDtable[1])};
	ata_writeBMR(ata, priBMR, secBMR);

	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_DEV, ATA_CMD_DEV_LBA_BIT); //set LBA bit
	ata_sleep();
	//LBA (low, mid, high) and sector count drive registers work as FIFOs
	//first we write the higher bytes, and then lower, to the same register
	//in the LBA Low: first bits 31:24, then 7:0
	//LBA Mid: 39:32, then 15:8
	//LBA High: 47:40, then 23:16
	//Sector count: 15:8, then 7:0
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_SECCNT, (sec & 0xFF00) >> 8);
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_LBA_LOW, (lba & 0xFF000000) >> 24);
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_LBA_MID, (lba & 0xFF00000000) >> 32);
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_LBA_HIGH, (lba & 0xFF0000000000) >> 40);
	ata_sleep();
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_SECCNT, sec & 0xFF);
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_LBA_LOW, lba & 0xFF);
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_LBA_MID, (lba & 0xFF00) >> 8);
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_LBA_HIGH, (lba & 0xFF0000) >> 16);
	port_writeByte(ata.cmdPort[chan] + ATA_CMD_PORT_CMD_STA, rw ? ATA_CMD_WRITEDMA : ATA_CMD_READDMA); //issue Read DMA extended (48-bit LBA) command or Write DMA extended command
	ata_sleep();



	ata_writeBMRcmd(ata, chan, (rw ? 0 : ATA_BMR_CMD_RWCON_BIT) | ATA_BMR_CMD_SSBM_BIT); //start the process
	ata_sleep();



	uint8_t sta = 0;
	while(1)
	{
		ata_readBMRstatus(ata, chan, &sta);
		if(sta & ATA_BMR_STA_INT_BIT) break; //wait for an interrupt
		if(sta & ATA_BMR_STA_ERR_BIT) break; //if an error occurred
	}

	ata_writeBMRcmd(ata, chan, 0); //clear command register
	ata_writeBMRstatus(ata, chan, ATA_BMR_STA_INT_BIT | ATA_BMR_STA_ERR_BIT); //clear status register


	//PIIX datasheet specifies 3 possible activity bit (bit 0) and interrupt bit (bit 2) combinations:
	//1. activity 1, interrupt 0 - transfer in progress
	//2. activity 1, interrupt 1 - device transfer size was smaller than region size specified in PRD table. This shouldn't happen in this implementation, so treat it as an error
	//3. activity 0, interrupt 1 - transfer completed
	//there can be an error bit set as well
	if((sta & ATA_BMR_STA_ACT_BIT) || (sta & ATA_BMR_STA_ERR_BIT)) //activity bit is set and interrupt bit is set (checked by the condition in the loop above), or error bit is set
	{
#if __DEBUG > 0
		printf("\nDisk read/write error occurred\n");
#endif
		return ATA_DMA_ERR;
	}

	return ATA_OK;
}


void ata_init(void)
{
	for(uint8_t i = 0; i < ATA_MAX_CONTROLLERS; i++)
	{
		ata_controllerList[i].present = 0;
		ata_controllerList[i].usable = 0;
		ata_controllerList[i].type = ATA_IDE_UNKNOWN;
		ata_controllerList[i].disk[0][0].present = 0;
		ata_controllerList[i].disk[0][1].present = 0;
		ata_controllerList[i].disk[1][0].present = 0;
		ata_controllerList[i].disk[1][1].present = 0;
	}
}


void ata_setAddDiskCallback(void (*callback)(void*, uint8_t))
{
	ata_addDiskCallback = callback;
}
