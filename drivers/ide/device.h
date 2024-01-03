#include "defines.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "ke/core/mutex.h"
#include <stdbool.h>

struct IdePrdEntry
{
    uint32_t address;
    uint16_t size;
    uint16_t eot;
} PACKED;

struct IdePrdTable
{
    struct IdePrdEntry *table;
    uint32_t physical;
};

#define IDE_DRIVE_SERIAL_NUMBER_SIZE 20
#define IDE_DRIVE_MODEL_NUMBER_SIZE 40

struct IdeDriveData
{
    char serial[IDE_DRIVE_SERIAL_NUMBER_SIZE + 1];
    char model[IDE_DRIVE_MODEL_NUMBER_SIZE + 1];
    uint8_t present : 1;
    uint8_t usable : 1;
    uint8_t lba48 : 1;
    uint64_t sectors;
    uint32_t sectorSize;
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

struct IdeDeviceData
{
    struct IoSubDeviceObject *enumerator;
    bool initialized;
    bool busMaster;
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
        struct IoDriverRp *rp;
        
        struct IoMemoryDescriptor nextBuffer;
        uint64_t remainingBytes;
        uint64_t nextLba;
        KeSpinlock lock;

        struct IdeDriveData drive[2];
    } channel[2];
};

STATUS IdeConfigureController(struct IoSubDeviceObject *device, struct IdeDeviceData *info);

STATUS IdeClearPrdTable(struct IdePrdTable *t);

STATUS IdeInitializePrdTables(struct IdeDeviceData *info);

uint32_t IdeAddPrdEntry(struct IdePrdTable *table, uint32_t address, uint16_t size);

STATUS IdeDetectAllDrives(struct IdeDeviceData *ide);

STATUS IdeCreateDriveDevice(struct IdeDriveData *drive, struct ExDriverObject *driver);

STATUS IdeCreateAllDriveDevices(struct IdeDeviceData *info, struct ExDriverObject *driver);

STATUS IdeIsr(void *context);

void IdeWriteBmrCommand(struct IdeDeviceData *info, uint8_t chan, uint8_t command);

uint8_t IdeReadBmrStatus(struct IdeDeviceData *info, uint8_t chan);

void IdeWriteBmrStatus(struct IdeDeviceData *info, uint8_t chan, uint8_t status);

void IdeWriteBmrPrdt(struct IdeDeviceData *info, uint8_t chan, struct IdePrdTable *prdt);

void IdeWriteLba28Parameters(struct IdeDeviceData *ide, uint8_t channel, uint8_t slot, uint32_t lba, uint8_t sectors);

void IdeWriteLba48Parameters(struct IdeDeviceData *ide, uint8_t channel, uint8_t slot, uint64_t lba, uint16_t sectors);

void IdeStartTransfer(struct IdeDeviceData *info, uint8_t channel, uint8_t slot, bool write, bool lba48);