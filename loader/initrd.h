#ifndef INITRD_H_
#define INITRD_H_

#define INITRD_PROVIDE_WRITE_ROUTINES
#define INITRD_PROVIDE_READ_ROUTINES

#include <stdint.h>

#define INITRD_MAX_FILE_NAME_LENGTH 32

typedef struct InitrdFileHeader_t
{
    char name[INITRD_MAX_FILE_NAME_LENGTH + 1]; //null terminated file name
    uint32_t size; //file size in bytes
    uint32_t nextFileOffset; //next file header offset starting from the beginning of this header. 0 if there is no next file
} InitrdFileHeader_t;

#define INITRD_MAGIC_NUMBER "INITRD"
#define INITRD_HEADER_SIZE 16

typedef struct InitrdHeader_t
{
    char magic[sizeof(INITRD_MAGIC_NUMBER)]; //initrd identification
    uint32_t firstFileOffset; //first file header offset starting from the beginning of the image
    char unused[INITRD_HEADER_SIZE - sizeof(INITRD_MAGIC_NUMBER)]; //unused fill bytes
} InitrdHeader_t;


#endif