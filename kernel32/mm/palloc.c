#include "palloc.h"
#include "rtl/string.h"
#include "hal/arch.h"
#include "hal/mm.h"
#include "ke/core/mutex.h"
#include "multiboot.h"
#include "ex/elf.h"

#define MM_BUDDY_SMALLEST PAGE_SIZE //smallest MmBuddy block size
#define MM_BUDDY_COUNT 8 //numer of buddies, MM_BUDDY_SMALLEST<<(N-1) will be the biggest block size
#define MM_BUDDY_ENTRIES (HAL_VIRTUAL_SPACE_SIZE / MM_BUDDY_SMALLEST / 32) //number of entries in order 0 MmBuddy


/**
 * @brief Find shift in MmBuddy table for given order
 * @param order Buddy order
*/
#define BUDDY_ORDER_SHIFT(order) ((order) ? (((PADDRESS)MM_BUDDY_ENTRIES * (((PADDRESS)1 << (PADDRESS)(order)) - (PADDRESS)1)) >> ((PADDRESS)(order) - (PADDRESS)1)) : (PADDRESS)0)

//MmBuddy allocator storage
//whole bitmap is stored as a 1D array to save memory
//with 2D array every MmBuddy must have the same length which wastes tons of memory
//of course the buddies could be separated to different variables, but this would be an unmaintainable mess
//just store all buddies consecutively so that the whole table is almost at it's minimum possible size
//use BUDDY() macro to access this storage
//to speed up the lookup every page is stored as 1 bit in 32-bit array elements
static uint32_t MmBuddy[BUDDY_ORDER_SHIFT(MM_BUDDY_COUNT)];

/**
 * @brief Buddy general access macro
 * @param order Buddy order
 * @param index Page number/index
 * 
 * Use this macro to access and manipulate buddies
*/
#define BUDDY(order, index) MmBuddy[BUDDY_ORDER_SHIFT(order) + (((PADDRESS)(index) >> (PADDRESS)(order)) >> (PADDRESS)5)]

/**
 * @brief Buddy element access macro
 * @param order Buddy order
 * @param element Index of MmBuddy element (not page index)
*/
#define BUDDY_ELEMENT(order, element) MmBuddy[BUDDY_ORDER_SHIFT(order) + (PADDRESS)(element)]

/**
 * @brief Buddy bit access macro
 * @param order Buddy order
 * @param Page number/index
 * 
 * Use this macro to get bit corresponding to given page from MmBuddy element
*/
#define BUDDY_BIT(order, index) ((uint32_t)1 << (((PADDRESS)(index) >> (PADDRESS)(order)) % (PADDRESS)32))

#define MM_MAX_USABLE_REGION_COUNT 16 //max count of usable region entries

/**
 * @brief Usable physical memory regions
 */
static struct MmMemoryPool MmUsableRegion[MM_MAX_USABLE_REGION_COUNT];

/**
 * @brief Number of usable physical memory regions
 */
static uint32_t MmUsableRegionCount = 0;

/**
 * @brief Architecture-specific-code-defined physical pools
 */
extern struct MmMemoryPool HalPhysicalPool[HAL_PHYSICAL_MEMORY_POOLS];

//TODO: this should be done differently
static KeSpinlock MmPhysicalAllocatorLock = KeSpinlockInitializer;

inline bool MmCheckIfPhysicalMemoryUsable(PADDRESS address, PSIZE size)
{
    for(uint32_t i = 0; i < MmUsableRegionCount; i++)
    {
        if((MmUsableRegion[i].base <= address) && ((MmUsableRegion[i].base + MmUsableRegion[i].size) >= (address + size)))
            return true;
    }
    return false;
}

inline bool MmCheckIfPhysicalMemoryInPool(PADDRESS address, PSIZE size, uint32_t pool)
{
    if(pool >= HAL_PHYSICAL_MEMORY_POOLS)
        return false;
    
    if((address >= HalPhysicalPool[pool].base) && ((address + size) <= (HalPhysicalPool[pool].base + HalPhysicalPool[pool].size)))
        return true;
    
    return false;
}

/**
 * @brief Find free block of given order
 * @param order Block order
 * @param *index Returned block index
 * @param pool Physical memory pool
 * @return True if found, false otherwise
*/
static bool MmFindBlock(uint8_t order, PADDRESS *index, uint32_t pool)
{
    if(MM_BUDDY_COUNT <= order)
        return false;
    
    PADDRESS i = (HalPhysicalPool[pool].base / (MM_BUDDY_SMALLEST << order)) >> 5; 
    PADDRESS end = (MM_BUDDY_ENTRIES >> order); //calculate number of entries for given order
    if(0 == end)
        end = 1;
    PADDRESS poolEnd = ((HalPhysicalPool[pool].base + HalPhysicalPool[pool].size) / (MM_BUDDY_SMALLEST << order)) >> 5;
    if(end > poolEnd)
        end = poolEnd;

    for(; i < end; i++) //loop for entries in given pool
    {
        if(0xFFFFFFFF != BUDDY_ELEMENT(order, i)) //check if there is at least one free block (32 bits at once)
        {
            for(uint8_t k = 0; k < 32; k++)
            {
                if(0 == (BUDDY_ELEMENT(order, i) & (1 << k))) //find free block
                {
                    *index =  ((i << 5) + k) * ((PADDRESS)1 << order); //get block index

                    if(!MmCheckIfPhysicalMemoryInPool((*index) * MM_BUDDY_SMALLEST, MM_BUDDY_SMALLEST << order, pool)
                        || !MmCheckIfPhysicalMemoryUsable((*index) * MM_BUDDY_SMALLEST, MM_BUDDY_SMALLEST << order))
                    {
                        
                    }
                    else
                    {
                        return true;
                    }
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
static void MmMarkAsUsed(PADDRESS index, PSIZE count)
{
    for(uint8_t i = 0; i < MM_BUDDY_COUNT; i++) //iterate through all orders
    {
        for(PSIZE k = 0; k < count; k++)
            BUDDY(i, index + (k << i)) |= BUDDY_BIT(i, index + (k << i)); //mark all blocks
        
        if((count & 1) || (0 == count))
        {
            count >>= 1;
            ++count;
        }
        else
        {
            count >>= 1;
        }
    }
}

/**
 * @brief Allocate best-fitting block
 * @param order Requested block order
 * @param *index Returned block index
 * @param pool Physical memory pool to allocate from
 * @return True if found, otherwise false
*/
static bool MmAllocateBlock(uint8_t order, PADDRESS *index, uint32_t pool)
{
    if(0 != MmFindBlock(order, index, pool))
    {
        MmMarkAsUsed(*index, 1 << order);
        return true;
    }
    return false;
}

/**
 * @brief Calculate required block order for given size
 * @param n Number of bytes
 * @return Block order
 * @attention If n is bigger than the biggest block size, the biggest block order is returned
*/
static uint8_t MmGetBlockOrderForSize(PSIZE n)
{
    PSIZE currentBlockSize = MM_BUDDY_SMALLEST;
    uint8_t order = 0;
    for(; order < MM_BUDDY_COUNT; order++)
    {
        if(n <= currentBlockSize)
            break;
        
        currentBlockSize <<= 1;
    }
    return order;
}

PSIZE MmAllocatePhysicalMemory(PSIZE n, PADDRESS *address)
{
    return MmAllocatePhysicalMemoryFromPool(n, address, MM_PHYSICAL_POOL_STANDARD);
}

inline PSIZE MmAllocatePhysicalMemoryFromPool(PSIZE n, PADDRESS *address, uint32_t pool)
{
    if(pool >= HAL_PHYSICAL_MEMORY_POOLS)
        return 0;

    uint8_t order = MmGetBlockOrderForSize(n); //calculate required order
    PADDRESS index = 0; //block index
    PRIO prio = KeAcquireSpinlock(&MmPhysicalAllocatorLock);
    barrier();
    while(0 == MmAllocateBlock(order, &index, pool)) //try to allocate biggest matching block
    {
        if(0 == order) //order 0 reached and no block was found - no memory
        {
            barrier();
            KeReleaseSpinlock(&MmPhysicalAllocatorLock, prio);
            *address = 0;
            return 0;
        }
        order--; //try smaller block if not found
    }
    
    barrier();
    KeReleaseSpinlock(&MmPhysicalAllocatorLock, prio);
    *address = index * MM_BUDDY_SMALLEST; //calculate physical address
    PSIZE actualSize = MM_BUDDY_SMALLEST << order; //calculate number of bytes actually allocated
    if(actualSize > n) //do not return bigger count than requested
        actualSize = n;
    
    return actualSize;
}

PSIZE MmAllocateContiguousPhysicalMemory(PSIZE n, PADDRESS *address, PSIZE align)
{
    return MmAllocateContiguousPhysicalMemoryFromPool(n, address, align, MM_PHYSICAL_POOL_STANDARD);
}

PSIZE MmAllocateContiguousPhysicalMemoryFromPool(PSIZE n, PADDRESS *address, PSIZE align, uint32_t pool)
{
    if(pool >= HAL_PHYSICAL_MEMORY_POOLS)
        return 0;

    PSIZE blocks = n / MM_BUDDY_SMALLEST + ((n % MM_BUDDY_SMALLEST) ? 1 : 0); //calculate number of blocks
    align /= PAGE_SIZE; //recalculate alignment value from bytes to page count

	PADDRESS firstBlock = 0;
    PADDRESS freeBlocks = 0;

    PRIO prio = KeAcquireSpinlock(&MmPhysicalAllocatorLock);
    barrier();

	for(PADDRESS i = (HalPhysicalPool[pool].base / MM_BUDDY_SMALLEST) >> 5; 
        i < (((HalPhysicalPool[pool].base + HalPhysicalPool[pool].size) / MM_BUDDY_SMALLEST) >> 5); 
        i++)
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
                            if(!MmCheckIfPhysicalMemoryInPool(firstBlock * MM_BUDDY_SMALLEST, freeBlocks * MM_BUDDY_SMALLEST, pool)
							    || !MmCheckIfPhysicalMemoryUsable(firstBlock * MM_BUDDY_SMALLEST, freeBlocks * MM_BUDDY_SMALLEST))
                            {
                                freeBlocks = 0;
                                continue;
                            }
                            
                            MmMarkAsUsed(firstBlock, blocks);
                            barrier();
                            KeReleaseSpinlock(&MmPhysicalAllocatorLock, prio);
							*address = firstBlock * MM_BUDDY_SMALLEST;
                            return blocks * MM_BUDDY_SMALLEST;
						}

						j++; //increment bit index
						if(j == 32) //all 32 bits checked?
						{
							j = 0; //reset bit index
							i++; //go to the next MmBuddy entry
						}

						if(i == MM_BUDDY_ENTRIES) //end of MmBuddy reached, allocation failed
                        {
                            barrier();
                            KeReleaseSpinlock(&MmPhysicalAllocatorLock, prio);
                            *address = 0;
							return 0;
                        }
					}
				}
			}
		}
	}
    barrier();
    KeReleaseSpinlock(&MmPhysicalAllocatorLock, prio);
    *address = 0;
	return 0;
}

/**
 * @brief Free memory block and collect higher order free blocks
 * @param index Block index
*/
static void MmFreeBlock(PADDRESS index)
{
    for(PADDRESS  i = 0; i < MM_BUDDY_COUNT; i++)
    {
        BUDDY(i, index) &= ~BUDDY_BIT(i, index);
        PADDRESS blockBuddy = index ^ ((PADDRESS)1 << i);
        if(BUDDY(i, blockBuddy) & BUDDY_BIT(i, blockBuddy))
            break;
    }
} 

void MmFreePhysicalMemory(PADDRESS address, PSIZE n)
{
    PADDRESS index = address / PAGE_SIZE; //calculate page number from address
    PSIZE blocks = n / MM_BUDDY_SMALLEST + ((n %  MM_BUDDY_SMALLEST) ? 1 : 0); //calculate number of blocks
    PRIO prio = KeAcquireSpinlock(&MmPhysicalAllocatorLock);
    barrier();
    for(PSIZE i = 0; i < blocks; i++)
    {
        MmFreeBlock(index + i); //free blocks
    }
    barrier();
    KeReleaseSpinlock(&MmPhysicalAllocatorLock, prio);
}

//linker-defined symbols for kernel image memory boundaries
extern char _kstart, _kend;

void MmInitPhysicalAllocator(struct Multiboot2InfoHeader *mb2h)
{
    //clear buddies
    RtlMemset(MmBuddy, 0, sizeof(MmBuddy));
    //iterate through the kernel image mappings and mark used pages
    for(PADDRESS i = ALIGN_DOWN((PADDRESS)(&_kstart), PAGE_SIZE); 
        i < ALIGN_UP((PADDRESS)(&_kend), PAGE_SIZE); 
        i += PAGE_SIZE)
    {
        PADDRESS physical;
        //obtain physical address
        if(OK == HalGetPhysicalAddress(i, &physical))
        {
            //if mapped, mark block as used
            MmMarkAsUsed(physical / MM_BUDDY_SMALLEST, 1);
        }
    }

    const struct Multiboot2InfoTag *tag = Multiboot2GetTag(mb2h, NULL);
    while(NULL != tag)
    {
        //store usable memory regions
        if(MB2_MEMORY_MAP == tag->type)
        {
            const struct Multiboot2MemoryMapTag *mm = (const struct Multiboot2MemoryMapTag*)tag;
            uint32_t count = (mm->header.size - sizeof(*mm)) / mm->entry_size;
            const struct Multiboot2MemoryMapEntry *e = (const struct Multiboot2MemoryMapEntry*)(mm + 1);
            while(0 != count)
            {
                if(MB2_MEMORY_AVAILABLE == e->type)
                {
                    if(MmUsableRegionCount < MM_MAX_USABLE_REGION_COUNT)
                    {
                        //align base up to page size
                        MmUsableRegion[MmUsableRegionCount].base = ALIGN_UP(e->base, PAGE_SIZE);
                        MmUsableRegion[MmUsableRegionCount].size = e->length;
                        //check if base changed after alignment
                        if(MmUsableRegion[MmUsableRegionCount].base != e->base)
                        {
                            //if so, the size also changed
                            uint64_t diff = MmUsableRegion[MmUsableRegionCount].base - e->base;
                            MmUsableRegion[MmUsableRegionCount].size -= diff;
                        }
                        //align size down to page size
                        MmUsableRegion[MmUsableRegionCount].size = ALIGN_DOWN(MmUsableRegion[MmUsableRegionCount].size, PAGE_SIZE);
                        //check if the region is at least one page size
                        if(MmUsableRegion[MmUsableRegionCount].size >= PAGE_SIZE)
                            ++MmUsableRegionCount;
                    }                
                }
                e = (const struct Multiboot2MemoryMapEntry*)((uintptr_t)e + mm->entry_size);
                --count;
            }
            
        }
        //mark frames occupied by the load modules
        else if(MB2_MODULE == tag->type)
        {
            const struct Multiboot2ModuleTag *mod = (const struct Multiboot2ModuleTag*)tag;
            uint32_t alignedSize = ALIGN_UP(mod->mod_end, MM_BUDDY_SMALLEST);
            alignedSize -= ALIGN_DOWN(mod->mod_start, MM_BUDDY_SMALLEST);
            MmMarkAsUsed(mod->mod_start / MM_BUDDY_SMALLEST, alignedSize / MM_BUDDY_SMALLEST);
        }
        //mark frames occupied by the kernel symbol and string tables
        else if(MB2_ELF_SYMBOLS == tag->type)
        {
            //reserve memory that stores kernel symbol and string tables
            const struct Multiboot2ElfSymbolsTag *elf = (const struct Multiboot2ElfSymbolsTag*)tag;
            uint16_t count = elf->num; //get section header count
            const struct Elf32_Shdr *s = (const struct Elf32_Shdr*)(elf + 1);
            while(0 != count)
            {
                if((SHT_SYMTAB == s->sh_type) || (SHT_STRTAB == s->sh_type))
                {
                    uint32_t alignedSize = ALIGN_UP(s->sh_addr + s->sh_size, MM_BUDDY_SMALLEST);
                    alignedSize -= ALIGN_DOWN(s->sh_addr, MM_BUDDY_SMALLEST);
                    MmMarkAsUsed(s->sh_addr / MM_BUDDY_SMALLEST, alignedSize / MM_BUDDY_SMALLEST);
                }
                s = (const struct Elf32_Shdr*)((uintptr_t)s + elf->entsize);
                --count;
            }
        }
        tag = Multiboot2GetTag(mb2h, tag);
    }
}

#if HAL_PHYSICAL_MEMORY_POOLS < 1
#error No physical memory pools defined
#endif