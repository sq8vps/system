#include <stdint.h>
#include "it/it.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "../drivers/vga/vga.h"

/**
 * @brief Parameters passed to the kernel by the bootloader
*/
struct KernelEntryArgs
{
	uint32_t kernelPageDir;
	uint32_t pageUsageTable;
} __attribute__ ((packed));

void main(struct KernelEntryArgs args)
{
	It_init();
	Mm_init(args.pageUsageTable, args.kernelPageDir);
	disp_clear();

	while(1);;
}
