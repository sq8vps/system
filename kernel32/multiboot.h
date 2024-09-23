#ifndef KERNEL_MULTIBOOT_H_
#define KERNEL_MULTIBOOT_H_

#include <stdint.h>
#include "defines.h"

#define MB2_TAG_ALIGNMENT 8

#define MB2_ATTR

struct Multiboot2Tag
{
    uint16_t type;
    uint16_t flags;
    uint32_t size;
} MB2_ATTR;

struct Multiboot2RelocatableTag
{
    struct Multiboot2Tag header;
    uint32_t min_addr;
    uint32_t max_addr;
    uint32_t align;
    uint32_t preference;
} MB2_ATTR;


struct Multiboot2Header
{
    uint32_t magic;
    uint32_t architecture;
    uint32_t header_length;
    uint32_t checksum;
} MB2_ATTR;

#define MULTIBOOT2_HEADER_MAGIC 0xE85250D6
#define MULTIBOOT2_HEADER_ARCHITECTURE_I386 0x00000000

enum Multiboot2InfoTagType
{
    MB2_BASIC_MEMORY = 4,
    MB2_BIOS_BOOT_DEVICE = 5,
    MB2_COMMAND_LINE = 1,
    MB2_MODULE = 3,
    MB2_ELF_SYMBOLS = 9,
    MB2_MEMORY_MAP = 6,
    MB2_BOOTLOADER_NAME = 2,
    MB2_APM_TABLE = 10,
    MB2_VBE_INFO = 7,
    MB2_FRAMEBUFFER = 8,
    MB2_EFI_32_SYSTEM_TABLE = 11,
    MB2_EFI_64_SYSTEM_TABLE = 12,
    MB2_SMBIOS_TABLES = 13,
    MB2_ACPI_OLD_RSDP = 14,
    MB2_ACPI_NEW_RSDP = 15,
    MB2_NETWORKING = 16,
    MB2_EFI_MEMORY_MAP = 17,
    MB2_EFI_BOOT_SERVICES_NOT_TERMINATED = 18,
    MB2_EFI_32_IMAGE = 19,
    MB2_EFI_64_IMAGE = 20,
    MB2_IMAGE_BASE = 21,
    MB2_TERMINATION_TAG = 0,
};

struct Multiboot2InfoHeader
{
    uint32_t total_size;
    uint32_t reserved;
} MB2_ATTR;

struct Multiboot2InfoTag
{
    uint32_t type;
    uint32_t size;
} MB2_ATTR;

struct Multiboot2ImageBaseTag
{
    struct Multiboot2InfoTag header;
    uint32_t base;
} MB2_ATTR;

enum Multiboot2MemoryMapEntryType
{
    MB2_MEMORY_AVAILABLE = 1,
    MB2_MEMORY_ACPI = 3,
    MB2_MEMORY_HIBERNATION_PRESERVED = 4,
    MB2_MEMORY_DEFECTIVE = 5,
};

struct Multiboot2MemoryMapTag
{
    struct Multiboot2InfoTag header;
    uint32_t entry_size;
    uint32_t entry_version;
} MB2_ATTR;

struct Multiboot2MemoryMapEntry
{
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} MB2_ATTR;

struct Multiboot2ModuleTag
{
    struct Multiboot2InfoTag header;
    uint32_t mod_start;
    uint32_t mod_end;
    char str[];
} MB2_ATTR;

struct Multiboot2ElfSymbolsTag
{
    struct Multiboot2InfoTag header;
    /*
    Apparently, the Multiboot2 specification (https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html) 
    is completely inconsistent on how this tag should look like. In specification, the following fields
    should be 16-bit and there should be a 16-bit "reserved" field. In the example code, however,
    all fields are 32-bit and there is no "reserved" field. It looks like Grub2 adheres to the latter.
    */
    uint32_t num;
    uint32_t entsize;
    uint32_t shndx;
} MB2_ATTR;

/**
 * @brief Find (next) Multiboot2 tag
 * @param *header Multiboot2 info header
 * @param *last Last tag or NULL to start from the beginning
 * @return Next tag or NULL if end of table reached
 */
const struct Multiboot2InfoTag *Multiboot2GetTag(const struct Multiboot2InfoHeader *header, 
    const struct Multiboot2InfoTag *last);

/**
 * @brief Find (next) Multiboot2 tag of given type
 * @param *header Multiboot2 info header
 * @param *last Last tag or NULL to start from the beginning
 * @param type Tag type to look for
 * @return Next tag of given type or NULL if end of table reached
 */
const struct Multiboot2InfoTag *Multiboot2FindTag(const struct Multiboot2InfoHeader *header, 
    const struct Multiboot2InfoTag *last, 
    enum Multiboot2InfoTagType type);


#endif