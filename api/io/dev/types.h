//This header file is generated automatically
#ifndef EXPORTED___API__IO_DEV_TYPES_H_
#define EXPORTED___API__IO_DEV_TYPES_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "defines.h"
#include "hal/interrupt.h"
enum IoBusType
{
    IO_BUS_TYPE_UNKNOWN = 0,
    IO_BUS_TYPE_ACPI,
    IO_BUS_TYPE_PCI,
    IO_BUS_TYPE_USB,
    IO_BUS_TYPE_ISA,
};

/**
 * @brief Bus location
*/
union IoBusId
{
    /**
     * @brief PCI address
    */
    struct
    {
        uint8_t bus;
        uint8_t device;
        uint8_t function;
    } pci;

    void *p; /**< Pointer */
    uint16_t u16; /**< 16-bit unsigned int */
    uint32_t u32; /**< 32-bit unsigned int */
    uint64_t u64; /**< 64-bit unsigned int */
};

/**
 * @brief Common PCI configuration space header
*/
struct IoPciDeviceHeader
{
    uint16_t vendor;
    uint16_t device;
    uint16_t command;
    uint16_t status;
    uint8_t revision;
    uint8_t progIf;
    uint8_t subclass;
    uint8_t classCode;
    uint8_t cache;
    uint8_t latency;
    uint8_t headerType;
    uint8_t bist;
    union
    {
        struct 
        {
            uint32_t bar[6];
            uint32_t cardbusCis;
            uint16_t subsystemVendor;
            uint16_t subsystemDevice;
            uint32_t expansionRom;
            uint8_t capabilities;
            uint8_t reserved[7];
            uint8_t interruptLine;
            uint8_t interruptPin;
            uint8_t minGrant;
            uint8_t maxLatency;
        } standard;
        struct
        {
            uint32_t bar[2];
            uint8_t primaryBus;
            uint8_t secondaryBus;
            uint8_t subordinateBus;
            uint8_t secondaryLatency;
            uint8_t ioBase;
            uint8_t ioLimit;
            uint16_t secondaryStatus;
            uint16_t memoryBase;
            uint16_t memoryLimit;
            uint16_t prefetchMemoryBase;
            uint16_t prefetchMemoryLimit;
            uint32_t prefetchBaseUpper;
            uint32_t prefetchLimitUpper;
            uint16_t ioBaseUpper;
            uint16_t ioLimitUpper;
            uint8_t capabilities; 
            uint8_t resereved[3];
            uint32_t expansionRom;
            uint8_t interruptLine;
            uint8_t interruptPin;
            uint16_t bridgeControl;
        } bridge;
        struct
        {
            uint32_t socketBase;
            uint8_t capabilitesOffset;
            uint8_t resreved;
            uint16_t secondaryStatus;
            uint8_t pciBus;
            uint8_t cardbusBus;
            uint8_t subordinateBus;
            uint8_t cardbusLatency;
            uint32_t memoryBase0;
            uint32_t memoryLimit0;
            uint32_t memoryBase1;
            uint32_t memoryLimit1;
            uint32_t ioBase0;
            uint32_t ioLimit0;
            uint32_t ioBase1;
            uint32_t ioLimit1;
            uint8_t interruptLine;
            uint8_t interruptPin;
            uint16_t bridgeControl;
            uint16_t subsystemDevice;
            uint16_t subsystemVendor;
            uint32_t legacyBase;
        } cardbus;
    };
} PACKED;

#define PCI_HEADER_TYPE_MASK 0x7F
#define PCI_HEADER_TYPE_MULTIFUNCTION 0x80

#define PCI_HEADER_COMMAND_IO_SPACE 0x01
#define PCI_HEADER_COMMAND_MEMORY_SPACE 0x02
#define PCI_HEADER_COMMAND_BUS_MASTER 0x04
#define PCI_HEADER_COMMAND_SPECIAL_CYCLES 0x08
#define PCI_HEADER_COMMAND_MEMORY_WRITE_INVALIDATE 0x10
#define PCI_HEADER_COMMAND_VGA_PALETTE_SNOOP 0x20
#define PCI_HEADER_COMMAND_PARITY_ERROR_RESPONSE 0x40
#define PCI_HEADER_COMMAND_STEPPING_CONTROL 0x80
#define PCI_HEADER_COMMAND_SERR 0x100
#define PCI_HEADER_COMMAND_FAST_BACK_BACK 0x200

#define PCI_HEADER_STATUS_CAPABILITES 0x10
#define PCI_HEADER_STATUS_66MHZ_CAPABLE 0x20
#define PCI_HEADER_STATUS_FAST_BACK_BACK_CAPABLE 0x80
#define PCI_HEADER_STATUS_MASTER_PARITY_ERROR 0x100
#define PCI_HEADER_STATUS_DEVSEL_MASK 0xE00
#define PCI_HEADER_STATUS_DEVSEL_FAST 0x000
#define PCI_HEADER_STATUS_DEVSEL_MEDIUM 0x200
#define PCI_HEADER_STATUS_DEVSEL_SLOW 0x400
#define PCI_HEADER_STATUS_SIGNALED_TARGET_ABORT 0x800
#define PCI_HEADER_STATUS_RECEIVED_TARGET_ABORT 0x1000
#define PCI_HEADER_STATUS_RECEIVED_MASTER_ABORT 0x2000
#define PCI_HEADER_STATUS_SIGNALED_SYSTEM_ERROR 0x4000
#define PCI_HEADER_STATUS_DETECTED_PARITY_ERROR 0x8000

struct IoMemoryDescriptor
{
    uintptr_t physical;
    void *mapped;
    uint64_t size;

    struct IoMemoryDescriptor *next;
};


#ifdef __cplusplus
}
#endif

#endif