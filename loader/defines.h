/*
 * defines.h
 *
 *  Created on: 27.12.2020
 *      Author: Piotr
 */

#ifndef LOADER_DEFINES_H_
#define LOADER_DEFINES_H_

#define EXTENDED_MEMORY_START 0x100000 //memory address at which the extended memory starts

#define LOADER_DATA 0x500 //mmeory address where data from 2nd stage bootloader is stored
#define DISK_SIG 0x500 //boot disk signature is stored here (4 bytes)



#define KERNEL_VIRTUAL_ADDRESS 0xE0000000
#define KERNEL_MEMORY_SIZE 0x1000000

#define DEFAULT_STACK_SIZE 0x100000

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef enum
{
	OK,

	ATA_INCORRECT_VAL,
	ATA_COUNT_LIMIT,
	ATA_DISK_TOO_SMALL,
	ATA_MEMORY_TOO_SMALL,
	ATA_NOT_ALIGNED,
	ATA_NO_DATA,
	ATA_DMA_ERR,
	ATA_DISK_NOT_PRESENT,

	DISK_NOT_PRESENT,
	DISK_NOT_PARTITIONED,

	NOT_IMPLEMENTED,

	PARTITION_EMPTY,
	PARTITION_TOO_SMALL,

	FAT_INVALID_VAL,
	FAT_DISK_NOT_PRESENT,
	FAT_NOT_FAT,
	FAT_NOT_INITIALIZED,
	FAT_NOT_FOUND,
	FAT_EOC,
	FAT_INVALID_CLUSTER,
	FAT_BROKEN_CLUSTER,

	ELF_BAD_ARCHITECTURE,
    ELF_BAD_INSTRUCTION_SET,
    ELF_BAD_FORMAT,
    ELF_BAD_ENDIANESS,
    ELF_BROKEN,
    ELF_UNSUPPORTED_TYPE,
    ELF_UNDEFINED_EXTERNAL_SYMBOL,
	ELF_NAME_TOO_LONG,
	ELF_MEMORY_TABLE_TOO_SMALL,

	MM_NO_MEMORY,
	MM_NOT_ALIGNED,
	MM_PAGE_NOT_PRESENT,
	MM_PAGE_ALREADY_MAPPED,

} error_t;

//debug level (amount of messages shown)
//0 - quiet (no messages)
//1 - user-friendly messages (standard)
//2 - full debug messages
#define __DEBUG 2

#if ((__DEBUG > 2) || (__DEBUG < 0))
#error "Debug level must be equal 0, 1 or 2"
#endif

#if MIN_KERNEL_MEMORY % 4096
#error "Minimum kernel memory size must be page (4096 bytes) aligned"
#endif

#endif /* LOADER_DEFINES_H_ */
