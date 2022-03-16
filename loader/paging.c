#include "paging.h"
#include "defines.h"

uint32_t pageDirectory[1024] __attribute__ ((aligned(4096))); //one page directory can address whole 4 GiB
uint32_t pageTable[PAGING_KERNEL_REQUIRED_TABLES + 1][1024] __attribute__ ((aligned(4096))); //one page table can address 4 MiB. Some tables are needed for kernel space + 1 table for identity paging the first 1 MiB


void Paging_construct(uint32_t kernelPhysicalAddress, uint32_t kernelVirtualAddress)
{
	for(uint16_t i = 0; i < 1024; i++)
	{
		pageDirectory[i] = 0; //zero all page directory entries
	}

	for(uint16_t i = 0; i < 256; i++) //set up identity paging for the lowest 1 MiB
	{
		pageTable[0][i] = (0x1000 * i) | 0x3; //set page frame address and flags: present, writable, supervisor only
	}
	pageDirectory[0] = (uint32_t)pageTable[0] | 0x3; //write page table address and set flags: present, writable, supervisor only

	//set up kernel pages
	for(uint16_t i = 0; i < PAGING_KERNEL_REQUIRED_TABLES; i++) //loop for each table (4 MiB)
	{
		for(uint16_t j = 0; j < 1024; j++) //loop for each page table entry (4 KiB)
		{
			pageTable[i + 1][j] = (kernelPhysicalAddress + 0x400000 * i + 0x1000 * j) | 0x3; //set physical page frame address and flags
		}
		pageDirectory[kernelVirtualAddress / 0x400000 + i] = (uint32_t)pageTable[i + 1]; //set page directory entry
	}
}

void Paging_enable(void)
{
	asm("mov cr3,eax" : : "a" ((uint32_t)pageDirectory)); //store page directory address in CR3
	asm("mov eax,cr0");
	asm("or eax,0x80000000"); //enable paging in CR0
	asm("mov cr0,eax");
}
