#include <stdint.h>
#include "device.h"
#include "defines.h"
#include "hal/ioport.h"
#include "ke/sched/sleep.h"

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

#define ATA_DEVICE_CONTROL_NIEN 0x02
#define ATA_DEVICE_CONTROL_SRST 0x04
#define ATA_DEVICE_CONTROL_HOB 0x80

#define ATA_COMMAND_IDENTIFY 0xEC
#define ATA_COMMAND_READ_DMA 0x25
#define ATA_COMMAND_WRITE_DMA 0x35

#define ATA_IDENTIFY_W63_DMA_MASK 0x7

static void selectDrive(struct IdeDeviceData *ide, uint8_t channel, uint8_t slot)
{
    if(ide->channel[channel].lastSelectedSlot != slot)
    {
        HalIoPortWriteByte(ide->channel[channel].cmdPort + ATA_REG_DEVICE, 
           ATA_DEVICE_RSVD_BITS | ((slot == PCI_IDE_SLOT_MASTER) ? ATA_DEVICE_MASTER_FLAG : ATA_DEVICE_SLAVE_FLAG));
        KeDelay(400);
    }
}


STATUS IdeDetectDrive(struct IdeDeviceData *ide, uint8_t channel, uint8_t slot)
{
    uint16_t cmdPort = ide->channel[channel].cmdPort;
    uint16_t controlPort = ide->channel[channel].controlPort;
    struct IdeDriveData *drive = &(ide->channel[channel].drive[slot]);

    selectDrive(ide, channel, slot);
    HalIoPortWriteByte(cmdPort + ATA_REG_SECTOR_COUNT, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_LBA_LOW, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_LBA_MID, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_LBA_HIGH, 0);
    HalIoPortWriteByte(cmdPort + ATA_REG_COMMAND, ATA_COMMAND_IDENTIFY);
    if(0 == HalIoPortReadByte(controlPort + ATA_REG_ALTERNATE_STATUS))
    {

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

            return OK;
        }
    }
    //assume drive usability
    drive->usable = true;
    //0-9
    //skip
    for(uint16_t i = 0; i < 10; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //10-19
    //serial number
    //20 character, but in 16-bit words. In each word there are two characters with swapped order (ATA....)
    for(uint16_t i = 0; i < 10; i++)
        ((uint16_t*)(drive->serial))[i] = 
            __builtin_bswap16(HalIoPortReadWord(cmdPort + ATA_REG_DATA));
    //20-62
    //skip
    for(uint16_t i = 0; i < 43; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //63
    //DMA capability
    if(0 == (HalIoPortReadWord(cmdPort + ATA_REG_DATA) & ATA_IDENTIFY_W63_DMA_MASK))
        drive->usable = false;
    //64-99
    //skip
    for(uint16_t i = 0; i < 36; i++)
        HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //100-103
    //number of sectors
    drive->sectors = HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    drive->sectors |= (uint64_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 16;
    drive->sectors |= (uint64_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 32;
    drive->sectors |= (uint64_t)HalIoPortReadWord(cmdPort + ATA_REG_DATA) << 48;
    //104, 105
    //skip
    HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    HalIoPortReadWord(cmdPort + ATA_REG_DATA);
    //106
    //sector size availability
    


STATUS IdeDetectAllDrives(struct IdeDeviceData *ide)
{
    
}