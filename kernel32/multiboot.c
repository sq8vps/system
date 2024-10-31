#include "multiboot.h"

/**
 * @brief Maximum size of information tags returned by the bootloader
 */
#define MULTIBOOT2_BUFFER_SIZE 4096

/**
 * @brief Bootloader information tags returned by the bootloader
 * @attention This buffer must be filled by the bootstrap code 
 */
uint8_t Multiboot2InfoBuffer[MULTIBOOT2_BUFFER_SIZE] ALIGN(8);


#define MULTIBOOT2_REQUEST_COUNT 4

struct
{
    struct Multiboot2Header header;
    ALIGN(MB2_TAG_ALIGNMENT) struct Multiboot2RelocatableTag relocatable;
    ALIGN(MB2_TAG_ALIGNMENT) struct Multiboot2Tag request;
    uint32_t requests[MULTIBOOT2_REQUEST_COUNT];
} 
ALIGN(16)
static volatile const Multiboot2Data __attribute__ ((section(".multiboot"))) =
{
    .header = 
    {
        .magic = MULTIBOOT2_HEADER_MAGIC,
        .architecture = MULTIBOOT2_HEADER_ARCHITECTURE_I386,
        .header_length = sizeof(Multiboot2Data),
        .checksum = -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_HEADER_ARCHITECTURE_I386 + sizeof(Multiboot2Data)),
    },
    .relocatable = //inform the bootloader that the image can be placed anywhere
    {
        .header = 
        {
            .type = 10, //relocatable image
            .size = 24, //constant
            .flags = 0, //"optional" bit not set
        },
        .min_addr = 0, //place image somewhere between 0 and virtual kernel space
        .max_addr = HAL_KERNEL_IMAGE_ADDRESS - 1,
        .align = PAGE_SIZE, //align to page size
        .preference = 0, //no location preference
    },
    .request = //pass information requests to the bootloader
    {
        .type = 1, //information request
        .size = MULTIBOOT2_REQUEST_COUNT * sizeof(uint32_t) + sizeof(Multiboot2Data.request),
        .flags = 0, //"optional" bit not set
    },
    .requests = 
    {
        MB2_COMMAND_LINE, //command line string
        MB2_BASIC_MEMORY, //amount of memory
        MB2_MEMORY_MAP, //memory map
        MB2_MODULE, //modules
    },
};

const struct Multiboot2InfoTag *Multiboot2FindTag(const struct Multiboot2InfoHeader *header, 
    const struct Multiboot2InfoTag *last, 
    enum Multiboot2InfoTagType type)
{
    do
    {
        last = Multiboot2GetTag(header, last);
        if(NULL != last)
        {
            if(type == last->type)
                return last;
        }
    }
    while (NULL != last);

    return NULL;
}

const struct Multiboot2InfoTag *Multiboot2GetTag(const struct Multiboot2InfoHeader *header, 
    const struct Multiboot2InfoTag *last)
{
    uint32_t size;
    struct Multiboot2InfoTag *tag;
    if(NULL != last)
    {
        size = header->total_size - ALIGN_UP((uintptr_t)last - (uintptr_t)header + last->size, MB2_TAG_ALIGNMENT);
        tag = (struct Multiboot2InfoTag *)(ALIGN_UP((uintptr_t)last + last->size, MB2_TAG_ALIGNMENT));
    }
    else
    {
        size = header->total_size;
        tag = (struct Multiboot2InfoTag *)(++header);
    }

    if((0 != size) && (MB2_TERMINATION_TAG != tag->type))
        return tag;
    else
        return NULL;
}