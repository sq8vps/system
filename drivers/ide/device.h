#include "defines.h"
#include "io/dev/dev.h"
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

struct IdeDriveData
{
    char serial[IDE_DRIVE_SERIAL_NUMBER_SIZE];
    bool usable;
    uint64_t sectors;
    uint16_t sectorSize;
};

#define PCI_IDE_CHANNEL_PRIMARY 0x00
#define PCI_IDE_CHANNEL_SECONDARY 0x01

#define PCI_IDE_SLOT_MASTER 0x00
#define PCI_IDE_SLOT_SLAVE 0x01

struct IdeDeviceData
{
    struct IoSubDeviceObject *enumerator;
    bool initialized;
    bool busMaster;
    struct
    {
        uint16_t cmdPort;
        uint16_t controlPort;
        uint16_t masterPort;
        uint8_t lastSelectedSlot : 1;
        struct IdePrdTable prdt;

        struct IdeDriveData drive[2];
    } channel[2];
};

STATUS IdeConfigureController(struct IoSubDeviceObject *device, struct IdeDeviceData *info);

STATUS IdeClearPrdTable(struct IdePrdTable *t);

STATUS IdeInitializePrdTables(struct IdeDeviceData *info);