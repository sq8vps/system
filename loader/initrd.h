#ifndef LOADER_INITRD_H_
#define LOADER_INITRD_H_

#include <stdint.h>
#include "defines.h"

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
error_t InitrdInit(uintptr_t address);

/**
 * @brief Prepare file header and space to write the file to
 * @param *name File name
 * @param size File size
 * @return Allocated address or 0 on failure
*/
uintptr_t InitrdPrepareSpaceForFile(char *name, uintptr_t size);

/**
 * @brief Get whole initial ramdisk size
 * @return Initrd size
*/
uintptr_t InitrdGetSize(void);

#endif