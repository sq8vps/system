#include <stdint.h>
#include "defines.h"
#include "hal/ioport.h"
#include "logging.h"
#include "ata.h"

#define ATA_REG_DATA 0x00
#define ATA_REG_ERROR 0x01
#define ATA_REG_FEATURES 0x01
#define ATA_REG_SECTOR_COUNT 0x02
#define ATA_REG_LBA_LOW 0x03
#define ATA_REG_LBA_MID 0x04
#define ATA_REG_LBA_HIGH 0x05
#define ATA_REG_DEVICE 0x06
#define ATA_REG_STATUS 0x07
#define ATA_REG_COMMAND 0x07

#define ATA_REG_ALTERNATE_STATUS 0x00
#define ATA_REG_DEVICE_CONTROL 0x00
#define ATA_REG_DRIVE_ADDRESS 0x01

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

#define ATA_DEVICE_MASTER_FLAG 0x00
#define ATA_DEVICE_SLAVE_FLAG 0x10
#define ATA_DEVICE_LBA_FLAG 0x40
#define ATA_DEVICE_RSVD_BITS 0xA0
#define ATA_DEVICE_LBA28_MASK 0x0F

#define ATA_DEVICE_CONTROL_NIEN 0x02
#define ATA_DEVICE_CONTROL_SRST 0x04
#define ATA_DEVICE_CONTROL_HOB 0x80

#define ATA_COMMAND_IDENTIFY 0xEC
#define ATA_COMMAND_READ_DMA_LBA48 0x25
#define ATA_COMMAND_WRITE_DMA_LBA48 0x35
#define ATA_COMMAND_READ_DMA_LBA28 0xC8
#define ATA_COMMAND_WRITE_DMA_LBA28 0xCA

static void selectDrive(struct IdeControllerData *ide, uint8_t channel, uint8_t slot)
{
    if(ide->channel[channel].lastSelectedSlot != slot)
    {
        HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_DEVICE, 
           ATA_DEVICE_RSVD_BITS | ATA_DEVICE_LBA_FLAG | ((slot == PCI_IDE_SLOT_MASTER) ? ATA_DEVICE_MASTER_FLAG : ATA_DEVICE_SLAVE_FLAG));
        for(uint8_t i = 0; i < 15; i++)
            HalIoPortReadByte(ide->channel[channel].controlPort + ATA_REG_ALTERNATE_STATUS);
        
        ide->channel[channel].lastSelectedSlot = slot;
    }
}

STATUS IdeDetectDrive(struct IdeControllerData *ide, uint8_t channel, uint8_t slot)
{
    uint16_t cmdPort = ide->channel[channel].cmdPort;
    uint16_t controlPort = ide->channel[channel].controlPort;

    selectDrive(ide, channel, slot);
    HalIoPortWriteByte(cmdPort + ATA_REG_SECTOR_COUNT, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_LBA_LOW, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_LBA_MID, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_LBA_HIGH, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_COMMAND, ATA_COMMAND_IDENTIFY);
    if(0 == HalIoPortReadByte(controlPort + ATA_REG_ALTERNATE_STATUS))
    {
        //drive not present
        return OK;
    }
    uint8_t status = ATA_STATUS_BSY;
    while(status & ATA_STATUS_BSY)
    {
        status = HalIoPortReadByte(controlPort + ATA_REG_ALTERNATE_STATUS);
        if(status & ATA_STATUS_DRQ)
            break;
        if((status & ATA_STATUS_ERR)
            || HalIoPortReadByte(cmdPort + ATA_REG_LBA_HIGH)
            || HalIoPortReadByte(cmdPort + ATA_REG_LBA_MID))
        {
            //drive not present
            return OK;
        }
    }
    //assume drive usability

    ide->channel[channel].drive[slot] = MmAllocateKernelHeapZeroed(sizeof(struct IdeDeviceData));
    if(NULL == ide->channel[channel].drive[slot])
        return OUT_OF_RESOURCES;
    
    ide->channel[channel].drive[slot]->isController = 0;
        
    struct IdeDriveData *drive = &(ide->channel[channel].drive[slot]->drive);

    drive->usable = 1;
    drive->present = 1;
    drive->controller = ide;
    //0-9
    //skip
    for(uint16_t i = 0; i < 10; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //10-19
    //serial number
    //20 characters, but in 16-bit words
    for(uint16_t i = 0; i < 10; i++)
        ((uint16_t*)(drive->serial))[i] = 
            __builtin_bswap16(HalIoPortReadWord(cmdPort + ATA_REG_DATA));
    //20-26
    //skip
    for(uint16_t i = 0; i < 7; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //27-46
    //model number
    //40 characters, but in 16-bit words
    for(uint16_t i = 0; i < 20; i++)
        ((uint16_t*)(drive->model))[i] = 
            __builtin_bswap16(HalIoPortReadWord(cmdPort + ATA_REG_DATA));
    //47-59
    //skip
    for(uint16_t i = 0; i < 13; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);      
    //60, 61
    //LBA28 number of sectors
    drive->sectors = HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    drive->sectors |= (uint64_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 16;
    //62
    //skip
    HalIoPortReadWord(cmdPort + ATA_REG_DATA); 
    //63
    //DMA capability
    if(0 == (HalIoPortReadWord(cmdPort + ATA_REG_DATA) & 0x07)) //check if any DMA mode available
        drive->usable = 0;
    //64-82
    //skip
    for(uint16_t i = 0; i < 19; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //83
    //LBA48 support
    if(HalIoPortReadWord(cmdPort + ATA_REG_DATA) & (1 << 10))
        drive->lba48 = 1;
    //84-99
    //skip
    for(uint16_t i = 0; i < 16; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //100-103
    //LBA48 number of sectors
    uint64_t lba48sectors = 0;
    lba48sectors = HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    lba48sectors |= (uint64_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 16;
    lba48sectors |= (uint64_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 32;
    lba48sectors |= (uint64_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 48;
    if(drive->lba48 && (lba48sectors > 0)) //use LBA48 only if available and if non-zero sector count is returned
        drive->sectors = lba48sectors;
    //104, 105
    //skip
    HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //106
    //sector size availability
    uint16_t logicalSectorStatus = HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //107-116
    //skip
    for(uint16_t i = 0; i < 10; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //117, 118
    //logical sector size
    //might not be valid, checked later
    drive->sectorSize = HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    drive->sectorSize |= (uint32_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 16;
    drive->sectorSize *= 2; //sector size was given in 16 bit words

    if(0x4000 == (logicalSectorStatus & 0xC000)) //check if field 106 was valid
    {
        if(0 == (logicalSectorStatus & (1 << 12))) //logical sector size field was invalid?
        {
            drive->sectorSize = 512; //assume 512 bytes
        }
    }
    else
    {
        drive->sectorSize = 512; //probably also assume 512?
    }
    //119-255
    //skip
    for(uint16_t i = 0; i < 137; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);

    if(drive->usable)
        LOG(SYSLOG_INFO, "Found drive at channel %d, slot %d, sector size %lu, %llu sectors", channel, slot, drive->sectorSize, drive->sectors);

    return OK;
}

STATUS IdeDetectAllDrives(struct IdeControllerData *ide)
{
    //force drive selection to save state 
    selectDrive(ide, PCI_IDE_CHANNEL_PRIMARY, PCI_IDE_SLOT_SLAVE);
    selectDrive(ide, PCI_IDE_CHANNEL_PRIMARY, PCI_IDE_SLOT_MASTER);
    selectDrive(ide, PCI_IDE_CHANNEL_SECONDARY, PCI_IDE_SLOT_SLAVE);
    selectDrive(ide, PCI_IDE_CHANNEL_SECONDARY, PCI_IDE_SLOT_MASTER);

    IdeDetectDrive(ide, PCI_IDE_CHANNEL_PRIMARY, PCI_IDE_SLOT_MASTER);
    IdeDetectDrive(ide, PCI_IDE_CHANNEL_PRIMARY, PCI_IDE_SLOT_SLAVE);
    IdeDetectDrive(ide, PCI_IDE_CHANNEL_SECONDARY, PCI_IDE_SLOT_MASTER);
    IdeDetectDrive(ide, PCI_IDE_CHANNEL_SECONDARY, PCI_IDE_SLOT_SLAVE);
    return OK;
}

void IdeWriteLba28Parameters(struct IdeControllerData *ide, uint8_t channel, uint8_t slot, uint32_t lba, uint8_t sectors)
{
    selectDrive(ide, channel, slot);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_LOW, lba & 0xFF);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_MID, lba >> 8);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_HIGH, lba >> 16);
    uint8_t t = HalIoPortReadByte(ide->channel[channel].cmdPort + ATA_REG_DEVICE) & ~ATA_DEVICE_LBA28_MASK;
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_DEVICE, t | ((lba >> 24) & ATA_DEVICE_LBA28_MASK));
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_SECTOR_COUNT, sectors);
}

void IdeWriteLba48Parameters(struct IdeControllerData *ide, uint8_t channel, uint8_t slot, uint64_t lba, uint16_t sectors)
{
    selectDrive(ide, channel, slot);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_LOW, lba >> 24);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_MID, lba >> 32);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_HIGH, lba >> 40);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_SECTOR_COUNT, sectors >> 8);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_LOW, lba & 0xFF);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_MID, lba >> 8);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_LBA_HIGH, lba >> 16);
    HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_SECTOR_COUNT, sectors & 0xFF);
}

void IdeStartTransfer(struct IdeControllerData *info, uint8_t channel, uint8_t slot, bool write, bool lba48)
{
    selectDrive(info, channel, slot);

    uint8_t command;
    if(lba48)
    {
        if(write)
            command = ATA_COMMAND_WRITE_DMA_LBA48;
        else
            command = ATA_COMMAND_READ_DMA_LBA48;  
    }
    else
    {
        if(write)
            command = ATA_COMMAND_WRITE_DMA_LBA28;
        else
            command = ATA_COMMAND_READ_DMA_LBA28;  
    }
    HalIoPortWriteByte(info->channel[channel].cmdPort + ATA_REG_COMMAND, command);
}