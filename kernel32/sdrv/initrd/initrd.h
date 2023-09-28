#ifndef KERNEL_INITRD_H_
#define KERNEL_INITRD_H_

#include <stdint.h>
#include "defines.h"
#include "io/fs/vfs.h"

#define INITRD_MAX_FILE_NAME_LENGTH 64
#define INITRD_MOUNT_POINT "initrd"

struct InitrdFileHeader
{
    char name[INITRD_MAX_FILE_NAME_LENGTH + 1]; //null terminated file name
    uint32_t size; //file size in bytes
    uint32_t nextFileOffset; //next file header offset starting from the beginning of this header. 0 if there is no next file
};

#define INITRD_MAGIC_NUMBER "INITRD"

struct InitrdHeader
{
    char magic[sizeof(INITRD_MAGIC_NUMBER)]; //initrd identification
    uint32_t firstFileOffset; //first file header offset starting from the beginning of the image. 0 if no files are present.
};

/**
 * @brief Initialize Initrd filesystem
 * @param address Initrd image address
 * @return Status code
*/
STATUS InitrdInit(uintptr_t address);

/**
 * @brief Deinitialize Initrd filesystem
 *
 * Deinitialize initrd filesystem so that it's image can be removed from the memory safely
*/
void InitrdDeinit(void);

/**
 * @brief Get file reference from initrd image
 * @param *name File name
 * @param *size Output file size
 * @return File reference
*/
uintptr_t InitrdGetFileReference(char *name, uint64_t *size);

/**
 * @brief Read file from initrd image
 * @param reference File reference from InitrdGetFileReference()
 * @param *buffer Destination buffer
 * @param size Maximum number of bytes to read
 * @param offset Offset within a file to start (in bytes)
 * @return Count of bytes actually read
*/
uint64_t InitrdReadFile(uintptr_t reference, void *buffer, uint64_t size, uint64_t offset);

#endif