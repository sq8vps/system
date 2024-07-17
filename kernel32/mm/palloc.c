#include "palloc.h"
#include "common.h"
#include "valloc.h"
#include "ke/core/mutex.h"

#define MM_BUDDY_SMALLEST MM_PAGE_SIZE //smallest buddy block size
#define MM_BUDDY_COUNT 8 //numer of buddies, MM_BUDDY_SMALLEST<<(N-1) will be the biggest block size
#define MM_BUDDY_ENTRIES (MM_MEMORY_SIZE / MM_BUDDY_SMALLEST / 32) //number of entries in order 0 buddy


/**
 * @brief Find shift in buddy table for given order
 * @param order Buddy order
*/
#define BUDDY_ORDER_SHIFT(order) ((order) ? (((uintptr_t)MM_BUDDY_ENTRIES * (((uintptr_t)1 << (uintptr_t)(order)) - (uintptr_t)1)) >> ((uintptr_t)(order) - (uintptr_t)1)) : (uintptr_t)0)

//buddy allocator storage
//whole bitmap is stored as a 1D array to save memory
//with 2D array every buddy must have the same length which wastes tons of memory
//of course the buddies could be separated to different variables, but this would be an unmaintainable mess
//just store all buddies consecutively so that the whole table is almost at it's minimum possible size
//use BUDDY() macro to access this storage
//to speed up the lookup every page is stored as 1 bit in 32-bit array elements
static uint32_t buddy[BUDDY_ORDER_SHIFT(MM_BUDDY_COUNT)];

/**
 * @brief Buddy general access macro
 * @param order Buddy order
 * @param index Page number/index
 * 
 * Use this macro to access and manipulate buddies
*/
#define BUDDY(order, index) buddy[BUDDY_ORDER_SHIFT(order) + (((uintptr_t)(index) >> (uintptr_t)(order)) >> (uintptr_t)5)]

/**
 * @brief Buddy element access macro
 * @param order Buddy order
 * @param element Index of buddy element (not page index)
*/
#define BUDDY_ELEMENT(order, element) buddy[BUDDY_ORDER_SHIFT(order) + (uintptr_t)(element)]

/**
 * @brief Buddy bit access macro
 * @param order Buddy order
 * @param Page number/index
 * 
 * Use this macro to get bit corresponding to given page from buddy element
*/
#define BUDDY_BIT(order, index) ((uint32_t)1 << (((uintptr_t)(index) >> (uintptr_t)(order)) % (uintptr_t)32))

/**
 * @brief A structure defining usable physical memory regions
*/
struct MmUsableRegion_s
{
    uintptr_t base;
    uintptr_t size;
};
#define MM_MAX_USABLE_REGION_ENTRY_COUNT 16 //max count of usable region entries
static struct MmUsableRegion_s usableRegions[MM_MAX_USABLE_REGION_ENTRY_COUNT]; //table of usable physical memory regions
uint16_t usableRegionCount = 0;

bool MmCheckIfPhysicalMemoryUsable(uintptr_t address, uintptr_t size)
{
    for(uint16_t i = 0; i < usableRegionCount; i++)
    {
        if((usableRegions[i].base <= address) && ((usableRegions[i].base + usableRegions[i].size) >= (address + size)))
            return true;
    }
    return false;
}

/**
 * @brief Find free block of given order
 * @param order Block order
 * @param index Returned block index
 * @return 1 if found, 0 if not found
*/
static bool findBlock(uint8_t order, uintptr_t *index)
{
    if(MM_BUDDY_COUNT <= order)
        return false;
    
    uintptr_t entries = (MM_BUDDY_ENTRIES >> order); //calculate number of entries for given order
    entries += entries ? 0 : 1;
    for(uintptr_t i = 0; i < entries; i++) //loop for all entries
    {
        if(0xFFFFFFFF != BUDDY_ELEMENT(order, i)) //check if there is at least one free block (32 bits at once)
        {
            for(uint8_t k = 0; k < 32; k++)
            {
                if(0 == (BUDDY_ELEMENT(order, i) & (1 << k))) //find free block
                {
                    *index =  ((i << 5) + k) * ((uintptr_t)1 << order); //get block index

                    if(0 == MmCheckIfPhysicalMemoryUsable((*index) * (MM_BUDDY_SMALLEST << order), MM_BUDDY_SMALLEST << order))
                        continue;

                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief Mark blocks as used
 * @param index Starting index
 * @param count Count of base (smallest - lowest order) blocks
*/
static void markAsUsed(uintptr_t index, uintptr_t count)
{
    for(uint8_t i = 0; i < MM_BUDDY_COUNT; i++) //iterate through all orders
    {
        for(uintptr_t k = 0; k < count; k++)
            BUDDY(i, index + (k << i)) |= BUDDY_BIT(i, index + (k << i)); //mark all blocks
        
        count >>= 1;
        if(0 == count)
            count = 1;
    }
}

static KeSpinlock blockAllocationMutex = KeSpinlockInitializer;

/**
 * @brief Allocate best-fitting block
 * @param order Requested block order
 * @param index Returned block index
 * @return 1 if found, 0 if not found
*/
static bool allocateBlock(uint8_t order, uintptr_t *index)
{
    KeAcquireSpinlock(&blockAllocationMutex);
    if(0 != findBlock(order, index))
    {
        markAsUsed(*index, 1 << order);
        KeReleaseSpinlock(&blockAllocationMutex);
        return true;
    }
    KeReleaseSpinlock(&blockAllocationMutex);
    return false;
}

/**
 * @brief Calculate required block order for given size
 * @param n Number of bytes
 * @return Block order
 * @attention If n is bigger than the biggest block size, the biggest block order is returned
*/
static uint8_t getBlockOrderForSize(uintptr_t n)
{
    uintptr_t currentBlockSize = MM_BUDDY_SMALLEST;
    uint8_t order = 0;
    for(; order < MM_BUDDY_COUNT; order++)
    {
        if(n <= currentBlockSize)
            break;
        
        currentBlockSize <<= 1;
    }
    return order;
}

uintptr_t MmAllocatePhysicalMemory(uintptr_t n, uintptr_t *address)
{
    uint8_t order = getBlockOrderForSize(n); //calculate required order
    uintptr_t index = 0; //block index
    while(0 == allocateBlock(order, &index)) //try to allocate biggest matching block
    {
        if(0 == order) //order 0 reached and no block was found - no memory
        {
            *address = 0;
            return 0;
        }
        order--; //try smaller block if not found
    }
    
    *address = index * MM_BUDDY_SMALLEST; //calculate physical address
    uintptr_t actualSize = MM_BUDDY_SMALLEST << order; //calculate number of bytes actually allocated
    if(actualSize > n) //do not return bigger count than requested
        actualSize = n;
    
    return actualSize;
}

static KeSpinlock contiguousAllocationMutex = KeSpinlockInitializer;

uintptr_t MmAllocateContiguousPhysicalMemory(uintptr_t n, uintptr_t *address, uintptr_t align)
{
    uintptr_t blocks = n / MM_BUDDY_SMALLEST + ((n %  MM_BUDDY_SMALLEST) ? 1 : 0); //calculate number of blocks
    align /= MM_PAGE_SIZE; //recalculate alignment value from bytes to page count

	uintptr_t firstBlock = 0;
    uintptr_t freeBlocks = 0;

    KeAcquireSpinlock(&contiguousAllocationMutex);

	for(uintptr_t i = 0; i < MM_BUDDY_ENTRIES; i++)
	{
		firstBlock = 0;
		if(0xFFFFFFFF != BUDDY_ELEMENT(0, i)) //if there is a free block (at least one bit is not set)
		{
			for(uint8_t j = 0; j < 32; j++)
			{
				if(0 == (BUDDY_ELEMENT(0, i) & (1 << j))) //bit is not set
				{
					firstBlock = (i << 5) + j; //store first possible block
                    if(align && (firstBlock % align)) //check if alignment request fulfilled
                        continue;
					freeBlocks = 0;
					while(0 == (BUDDY_ELEMENT(0, i) & (1 << j))) //loop for consecutive free blocks
					{
						freeBlocks++;
						if(freeBlocks == blocks) //enough blocks were found
						{
							if(0 == MmCheckIfPhysicalMemoryUsable(firstBlock * MM_BUDDY_SMALLEST, freeBlocks * MM_BUDDY_SMALLEST))
                            {
                                freeBlocks = 0;
                                continue;
                            }
                            
                            markAsUsed(firstBlock, blocks);
							*address = firstBlock * MM_BUDDY_SMALLEST;
                            KeReleaseSpinlock(&contiguousAllocationMutex);
                            return blocks * MM_BUDDY_SMALLEST;
						}

						j++; //increment bit index
						if(j == 32) //all 32 bits checked?
						{
							j = 0; //reset bit index
							i++; //go to the next buddy entry
						}

						if(i == MM_BUDDY_ENTRIES) //end of buddy reached, allocation failed
                        {
                            *address = 0;
                            KeReleaseSpinlock(&contiguousAllocationMutex);
							return 0;
                        }
					}
				}
			}
		}
	}
    *address = 0;
    KeReleaseSpinlock(&contiguousAllocationMutex);
	return 0;
}

static KeSpinlock freeBlockMutex = KeSpinlockInitializer;

/**
 * @brief Free memory block and collect higher order free blocks
 * @param index Block index
*/
static void freeBlock(uintptr_t index)
{
    KeAcquireSpinlock(&freeBlockMutex);
    for(uintptr_t i = 0; i < MM_BUDDY_COUNT; i++)
    {
        BUDDY(i, index) &= ~BUDDY_BIT(i, index);
        uintptr_t blockBuddy = index ^ ((uintptr_t)1 << i);
        if(BUDDY(i, blockBuddy) & BUDDY_BIT(i, blockBuddy))
            break;
    }
    KeReleaseSpinlock(&freeBlockMutex);
} 

void MmFreePhysicalMemory(uintptr_t address, uintptr_t n)
{
    //check if memory can actually be freed
    if(0 == MmCheckIfPhysicalMemoryUsable(address, MM_BUDDY_SMALLEST * n))
        return;

    uintptr_t index = address / MM_PAGE_SIZE; //calculate page number from address
    uintptr_t blocks = n / MM_BUDDY_SMALLEST + ((n %  MM_BUDDY_SMALLEST) ? 1 : 0); //calculate number of blocks
    for(uintptr_t i = 0; i < blocks; i++)
    {
        freeBlock(index + i); //free blocks
    }
}

void MmInitPhysicalAllocator(struct KernelEntryArgs *kernelArgs)
{
    //clear buddies
    CmMemset(buddy, 0, sizeof(buddy));
    //iterate through the existing page tables and check which page frames are really used
    uintptr_t vAddress = 0, pAddress = 0;
    while(1)
    {
        //check if given virtual address is mapped to any physical address
        if(OK == MmGetPhysicalPageAddress(vAddress, &pAddress))
        {
            //if so, mark block as used
            markAsUsed(pAddress / MM_BUDDY_SMALLEST, 1);
        }

        if((MM_MEMORY_SIZE - MM_PAGE_SIZE) == vAddress)
            break;
        
        vAddress += MM_PAGE_SIZE;
    } 

    //read usable memory regions and store them
    for(uint16_t i = 0; i < kernelArgs->biosMemoryMapSize; i++)
    {
        if(kernelArgs->biosMemoryMap[i].attributes != MM_BIOS_MEMORY_MAP_USABLE_REGION_ATTRIBUTES)
            continue; //drop unusable region
        
        if(kernelArgs->biosMemoryMap[i].length & (MM_PAGE_SIZE - 1)) //if memory address is not aligned to page size
		{
			uint16_t shift = MM_PAGE_SIZE - (kernelArgs->biosMemoryMap[i].length & (MM_PAGE_SIZE - 1)); //calculate required shift to align
			kernelArgs->biosMemoryMap[i].base += shift;
			kernelArgs->biosMemoryMap[i].length -= shift;

			if(kernelArgs->biosMemoryMap[i].length < MM_PAGE_SIZE)
				continue; //drop regions that are smaller than single page

			if(kernelArgs->biosMemoryMap[i].length & (MM_PAGE_SIZE - 1)) //check if size is a multiple of the page size
			{
				//if not, align the size again
				shift = MM_PAGE_SIZE - (kernelArgs->biosMemoryMap[i].length & (MM_PAGE_SIZE - 1));
				kernelArgs->biosMemoryMap[i].length -= shift;
			}
		}

        if(kernelArgs->biosMemoryMap[i].length < MM_PAGE_SIZE)
			continue; //drop regions that are smaller than single page
        
        if(kernelArgs->biosMemoryMap[i].base >= MM_MEMORY_SIZE)
            continue; //drop regions that are outside the memory space
        
        //check if region extends above the memory space
        if((kernelArgs->biosMemoryMap[i].base + kernelArgs->biosMemoryMap[i].length) > MM_MEMORY_SIZE)
        {
            //if so, it must be restricted
            uint64_t remainder = MM_MEMORY_SIZE - (kernelArgs->biosMemoryMap[i].base + kernelArgs->biosMemoryMap[i].length);
            kernelArgs->biosMemoryMap[i].length -= remainder;
        }

        usableRegions[usableRegionCount].base = kernelArgs->biosMemoryMap[i].base;
        usableRegions[usableRegionCount++].size = kernelArgs->biosMemoryMap[i].length;
        if(usableRegionCount == MM_MAX_USABLE_REGION_ENTRY_COUNT)
            return;
    }
}