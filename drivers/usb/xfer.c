#include "xfer.h"
#include "hc.h"
#include "mm/palloc.h"
#include "mm/dynmap.h"
#include "hal/i686/memory.h"
#include "rtl/string.h"

#define UHCI_FRAME_LIST_ENTRY_COUNT 1024

#define UHCI_LP_MASK 0xFFFFFFF0 /**< Link pointer mask */
#define UHCI_LP_VF (1 << 2) /**< Depth/breadth first mask - TD only */
#define UHCI_LP_Q (1 << 1) /**< QH/TD select mask */
#define UHCI_LP_T (1 << 0) /**< Terminate mask */

#define UHCI_CNTL_SPD (1 << 29) /**< Short packet detect mask */
#define UHCI_CNTL_ECNT_MASK (0x3 << 27) /**< Error counter mask */
#define UHCI_CNTL_LS (1 << 26) /**< Low speed device mask */
#define UHCI_CNTL_IOS (1 << 25) /**< Isochronous transfer descriptor mask */
#define UHCI_CNTL_IOC (1 << 24) /**< Interrupt on complete mask */
#define UHCI_CNTL_STATUS_MASK (0xFF << 16) /**< Status mask */
#define UHCI_CNTL_ACTLEN_MASK (0x7FF) /**< Actual length mask */

#define UHCI_TOKEN_MAXLEN_MASK (0x7FF << 21) /**< Maximum length mask */
#define UHCI_TOKEN_D (1 << 21) /**< Data toggle mask */
#define UCHI_TOKEN_ENDPT_MASK (0xF << 15) /**< Endpoint number mask */
#define UHCI_TOKEN_ADDR_MASK (0x7F << 8) /**< Device address mask */
#define UHCI_TOKEN_PID_MASK (0xFF) /**< Packed identification mask */

#define UHCI_STATUS_ACTIVE (1 << 23) /**< Transaction active */
#define UHCI_STATUS_STALLED (1 << 22) /**< Serious error at the device */
#define UHCI_STATUS_DATA_BUFFER_ERROR (1 << 21) /**< Overrun or underrun */
#define UHCI_STATUS_BABBLE (1 << 20) /**< Babble detected */
#define UHCI_STATUS_NAK (1 << 19) /**< Not acknowledge */
#define UHCI_STATUS_CRC_TIMEOUT (1 << 18) /**< Timeout or CRC error */
#define UHCI_STATUS_BITSTUFF_ERROR (1 << 17) /**< Bit stuffing violation */

typedef uint32_t UhciFrameListPointer;

struct UhciTd
{
    uint32_t linkPointer; /**< Transfer Descriptor link pointer */
    uint32_t controlAndStatus; /**< TD control and status */
    uint32_t token; /**< TD token */
    uint32_t buffer; /**< Buffer pointer */
    uint32_t data[4]; /**< Software-defined data */
} PACKED;

struct UhciQh
{
    uint32_t qhLinkPointer; /**< Queue Head link pointer */
    uint32_t qeLinkPointer; /**< Queue Element link pointer */
} PACKED;

STATUS UhciAllocateFrameList(struct UhciControllerInfo *info)
{
    PADDRESS flpPhysical = 0;
    size_t size = sizeof(UhciFrameListPointer) * UHCI_FRAME_LIST_ENTRY_COUNT;
    
    if(0 == MmAllocateContiguousPhysicalMemoryFromPool(size, &flpPhysical, 4096, I686_PHYSICAL_POOL_PCI_DMA))
        return OUT_OF_RESOURCES;
    
    info->frameList = MmMapDynamicMemory(flpPhysical, size, MM_FLAG_CACHE_DISABLE | MM_FLAG_WRITE_THROUGH);
    if(nullptr == info->frameList)
    {
        MmFreePhysicalMemory(flpPhysical, size);
        return OUT_OF_RESOURCES;
    }

    memset(info->frameList, 0, size);
    for(size_t i = 0; i < UHCI_FRAME_LIST_ENTRY_COUNT; i++)
        ((UhciFrameListPointer*)info->frameList)[i] |= UHCI_LP_T;

    return OK;
}