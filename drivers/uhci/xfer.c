#include "xfer.h"
#include "hc.h"
#include "mm/palloc.h"
#include "mm/dynmap.h"
#include "hal/i686/memory.h"
#include "rtl/string.h"
#include "mm/slab.h"
#include "hal/arch.h"
#include "if.h"
#include "logging.h"
#include <inttypes.h>
#include "mm/mm.h"

constexpr size_t UHCI_DEFAULT_TRANSFER_SIZE = 64 * 1024; /**< Default maximum size of a single transfer */
constexpr size_t UHCI_MAX_TRANSFER_SIZE = 16 * 1024 * 1024; /**< Absolute maximum size of a single transfer */
constexpr uint32_t UHCI_HOST_DELAY = 700; /**< Used to calculate bus load as per USB spec; in nanoseconds */
constexpr size_t UHCI_FRAME_LIST_ENTRY_COUNT = 1024; /**< Number of frame list entries = 1024 for UHCI */

#define UHCI_LP_MASK 0xFFFFFFF0 /**< Link pointer mask */
#define UHCI_LP_VF (1 << 2) /**< Depth/breadth first mask - TD only */
#define UHCI_LP_Q (1 << 1) /**< QH/TD select mask */
#define UHCI_LP_T (1 << 0) /**< Terminate mask */

#define UHCI_CNTL_SPD (1 << 29) /**< Short packet detect mask */
#define UHCI_CNTL_ECNT_SHIFT 27 /**< Error counter shift */
#define UHCI_CNTL_ECNT_MASK (0x3 << UHCI_CNTL_ECNT_SHIFT) /**< Error counter mask */
#define UHCI_CNTL_LS (1 << 26) /**< Low speed device mask */
#define UHCI_CNTL_IOS (1 << 25) /**< Isochronous transfer descriptor mask */
#define UHCI_CNTL_IOC (1 << 24) /**< Interrupt on complete mask */
#define UHCI_CNTL_STATUS_MASK (0xFF << 16) /**< Status mask */
#define UHCI_CNTL_ACTLEN_MASK (0x7FF) /**< Actual length mask */
#define UHCI_CNTL_STATUS_ACTIVE (1 << 23) /**< Transaction is active */

#define UHCI_TOKEN_MAXLEN_SHIFT 21 /**< Maximum length shift */
#define UHCI_TOKEN_MAXLEN_MASK (0x7FF << UHCI_TOKEN_MAXLEN_SHIFT) /**< Maximum length mask */
#define UHCI_TOKEN_D (1 << 19) /**< Data toggle mask */
#define UHCI_TOKEN_ENDPT_SHIFT 15 /**< Endpoint number shift */
#define UHCI_TOKEN_ENDPT_MASK (0xF << UHCI_TOKEN_ENDPT_SHIFT) /**< Endpoint number mask */
#define UHCI_TOKEN_ADDR_SHIFT 8 /**< Device address shift */
#define UHCI_TOKEN_ADDR_MASK (0x7F << UHCI_TOKEN_ADDR_SHIFT) /**< Device address mask */
#define UHCI_TOKEN_PID_MASK (0xFF) /**< Packed identification mask */

#define UHCI_TOKEN_PID_IN (0x69) /**< IN token */
#define UHCI_TOKEN_PID_OUT (0xE1) /**< OUT token */
#define UHCI_TOKEN_PID_SETUP (0x2D) /**< SETUP token */

#define UHCI_STATUS_ACTIVE (1 << 23) /**< Transaction active */
#define UHCI_STATUS_STALLED (1 << 22) /**< Serious error at the device */
#define UHCI_STATUS_DATA_BUFFER_ERROR (1 << 21) /**< Overrun or underrun */
#define UHCI_STATUS_BABBLE (1 << 20) /**< Babble detected */
#define UHCI_STATUS_NAK (1 << 19) /**< Not acknowledge */
#define UHCI_STATUS_CRC_TIMEOUT (1 << 18) /**< Timeout or CRC error */
#define UHCI_STATUS_BITSTUFF_ERROR (1 << 17) /**< Bit stuffing violation */

typedef uint32_t UhciFrameListPointer;

struct UhciTdHw
{
    uint32_t linkPointer; /**< Transfer Descriptor link pointer */
    volatile uint32_t controlAndStatus; /**< TD control and status */
    uint32_t token; /**< TD token */
    uint32_t buffer; /**< Buffer pointer */
    //4 32-bit dwords here for software use
} PACKED;

struct UhciQhHw
{
    uint32_t qhLinkPointer; /**< Queue Head link pointer */
    volatile uint32_t qeLinkPointer; /**< Queue Element link pointer */
} PACKED;

struct UhciTd
{
    struct UhciTdHw td; /**< TD as expected by the hardware */
    PADDRESS physical; /**< Physical address of this structure */
    struct UhciTd *next; /**< Next TD */
    struct UhciTd *previous; /**< Previous TD */
};

struct UhciQh
{
    struct UhciQhHw qh; /**< QH as expected by the hardware */
    PADDRESS physical; /**< Physical address of this structure */
    struct UhciQh *next; /**< Next horizontal QH */
    struct UhciQh *previous; /**< Previous horizontal QH */
};

constexpr uint32_t UHCI_EP_MAGIC = 0x50454855; //"UHEP" in LE

struct UhciEndpoint
{
    uint32_t magic; /**< Magic structure number */
    struct UsbEndpoint config; /**< Endpoint configuration */
    struct UhciQh *qh; /**< Queue head for this endpoint */
    struct UhciTd *finished; /**< List of finished TDs */
    uint32_t busTime; /**< Maximum required bus time in ns */
    uint16_t interval; /**< Polling interval */
    bool nextToggle; /**< Next toggle bit value */
    struct MmMemoryDescriptor *buffer; /**< Intermediate buffer */
    size_t firstSlot; /**< First slot number for interrupt endpoints */
};

struct UhciFrame
{
    struct
    {
        struct UhciTd *head; /**< Head of the isochronous TDs */
        struct UhciTd *tail; /**< Tail of the isochronous TDs */
    } isochronous;
    struct
    {
        struct UhciEndpoint *head; /**< Head of the interrupt endpoint list */
        struct UhciEndpoint *tail; /**< Tail of the interrupt endpoint list */
    } interrupt;
    uint64_t busTime; /**< Total bus time occupied by isochronous and interrupt transfers in ns */
};

STATUS UhciAllocateStructures(struct UhciControllerInfo *info)
{
    STATUS status = OK;
    PADDRESS flpPhysical = 0;
    size_t size = sizeof(UhciFrameListPointer) * UHCI_FRAME_LIST_ENTRY_COUNT;

    info->frameList = nullptr;
    info->tdSlab = nullptr;
    info->qhSlab = nullptr;
    
    if(0 == MmAllocateContiguousPhysicalMemoryFromPool(size, &flpPhysical, 4096, I686_PHYSICAL_POOL_PCI_DMA))
        return OUT_OF_RESOURCES;
    
    info->frameList = MmMapDynamicMemory(flpPhysical, size, MM_FLAG_CACHE_DISABLE | MM_FLAG_WRITE_THROUGH);
    if(nullptr == info->frameList)
    {
        status = OUT_OF_RESOURCES;
        goto leave;
    }

    info->frames = MmAllocateKernelHeapZeroed(sizeof(*info->frames) * UHCI_FRAME_LIST_ENTRY_COUNT);
    if(nullptr == info->frames)
    {
        status = OUT_OF_RESOURCES;
        goto leave;
    }

    memset(info->frameList, 0, size);
    for(size_t i = 0; i < UHCI_FRAME_LIST_ENTRY_COUNT; i++)
        ((UhciFrameListPointer*)info->frameList)[i] |= UHCI_LP_T;

    info->qhSlab = MmSlabCreateP(ALIGN_UP(sizeof(struct UhciQh), 16), I686_PHYSICAL_POOL_PCI_DMA, 0);
    if(nullptr == info->qhSlab)
    {
        status = OUT_OF_RESOURCES;
        goto leave;
    }

    info->tdSlab = MmSlabCreateP(ALIGN_UP(sizeof(struct UhciTd), 16), I686_PHYSICAL_POOL_PCI_DMA, 0);
    if(nullptr == info->tdSlab)
    {
        status = OUT_OF_RESOURCES;
        goto leave;
    }

leave:
    if(OK != status)
    {
        MmSlabDestroy(info->tdSlab);
        MmSlabDestroy(info->qhSlab);
        MmFreeKernelHeap(info->frames);
        MmUnmapDynamicMemory(info->frameList);
        MmFreePhysicalMemory(flpPhysical, size);
    }

    return status;
}

STATUS UhciOpenEndpoint(struct UsbHcd *hcd, struct UsbEndpoint *config, void **handle)
{
    STATUS status = OK;

    if(unlikely(nullptr == hcd) || unlikely(nullptr == config) || unlikely(nullptr == handle))
        return BAD_PARAMETER;

    struct UhciControllerInfo *info = hcd->hcdData;
    struct UhciEndpoint *ep = nullptr;
    uint32_t bitsTime10 = 0; //bus time calculation factor
    uint64_t busTime = 0; //bus time in ns
    size_t bestSlot = 0; //best first slot for interrupt transfers

    if(unlikely((USB_XFER_IN != config->direction) && (USB_XFER_OUT != config->direction)))
        return BAD_PARAMETER;

    if((USB_EP_ISOCHRONOUS == config->type) && (config->maxDataSize > config->maxPacketSize))
        return BAD_PARAMETER;

    if(USB_LOW_SPEED == config->speed)
    {
        switch(config->type)
        {
            case USB_EP_CONTROL:
                if(8 != config->maxPacketSize)
                    return BAD_PARAMETER;
                break;
            case USB_EP_INTERRUPT:
                if(config->maxPacketSize > 8)
                    return BAD_PARAMETER;
                if((config->period > 255) || (config->period < 10))
                    return BAD_PARAMETER;
                break;
            case USB_EP_BULK:
            case USB_EP_ISOCHRONOUS:
            default:
                return BAD_PARAMETER;
        }
    }
    else if(USB_FULL_SPEED == config->speed)
    {
        switch(config->type)
        {
            case USB_EP_CONTROL:
            case USB_EP_BULK:
                if((config->maxPacketSize > 64) || (config->maxPacketSize < 8) || (stdc_count_ones(config->maxPacketSize) > 1))
                    return BAD_PARAMETER;
                break;
            case USB_EP_INTERRUPT:
                if(config->maxPacketSize > 64)
                    return BAD_PARAMETER;
                if((config->period > 255) || (config->period < 1))
                    return BAD_PARAMETER;
                break;
            case USB_EP_ISOCHRONOUS:
                if(config->maxPacketSize > 1023)
                    return BAD_PARAMETER;
                break;
            default:
                return BAD_PARAMETER;
        }
    }
    else
        return NOT_SUPPORTED;

    bitsTime10 = 31 + 10 * ((7 * 8 * config->maxPacketSize) / 6);

    if(USB_EP_ISOCHRONOUS == config->type)
    {
        if(USB_XFER_IN == config->direction)
            busTime = 7268 + (8354 * bitsTime10) / 1000 + UHCI_HOST_DELAY;
        else
            busTime = 6265 + (8354 * bitsTime10) / 1000 + UHCI_HOST_DELAY;

        uint64_t timeLeft = (uint64_t)900000000 - busTime; //max. 90% of 1ms frame = 900us might be used for isochronous and interrupt transfers
        for(size_t i = 0; i < UHCI_FRAME_LIST_ENTRY_COUNT; i++)
        {
            if(info->frames[i].busTime > timeLeft)
                return OUT_OF_RESOURCES;
        }
    }
    else
    {
        if(USB_FULL_SPEED == config->speed)
            busTime = 9107 + (8354 * bitsTime10) / 1000 + UHCI_HOST_DELAY;
        else //USB_LOW_SPEED
        {
            if(USB_XFER_IN == config->direction)
                busTime = 64060 + 667 + (67667 * bitsTime10) / 1000 + UHCI_HOST_DELAY;
            else //USB_EP_OUT
                busTime = 64107 + 667 + (667 * bitsTime10) / 10 + UHCI_HOST_DELAY;
        }

        if(USB_EP_INTERRUPT == config->type)
        {
            size_t interval = stdc_bit_floor(config->period);
            uint64_t lowestOccupancy = UINT64_MAX;
            uint64_t timeLeft = (uint64_t)900000000 - busTime; //max. 90% of 1ms frame = 900us might be used for isochronous and interrupt transfers
            for(size_t i = 0; i < interval; i++)
            {
                uint64_t occupancy = 0;
                for(size_t k = 0; k < (UHCI_FRAME_LIST_ENTRY_COUNT / interval); k++)
                {
                    if(info->frames[(i + (interval * k)) % UHCI_FRAME_LIST_ENTRY_COUNT].busTime > timeLeft)
                    {
                        goto skipSlot;
                    }
                    occupancy += info->frames[(i + (interval * k)) % UHCI_FRAME_LIST_ENTRY_COUNT].busTime;
                }

                if(occupancy < lowestOccupancy)
                {
                    lowestOccupancy = occupancy;
                    bestSlot = i;
                }

                skipSlot:
            }

            if(UINT64_MAX == lowestOccupancy)
            {
                return OUT_OF_RESOURCES;
            }
        }
    }

    ep = MmAllocateKernelHeapZeroed(sizeof(*ep));
    if(nullptr == ep)
        return OUT_OF_RESOURCES;
    
    ep->magic = UHCI_EP_MAGIC;
    ep->config = *config;


    if(USB_EP_ISOCHRONOUS != config->type)
    {
        PADDRESS qhPhysical = 0;
        ep->qh = MmSlabAllocateP(info->qhSlab, &qhPhysical);
        if(nullptr == ep->qh)
        {
            MmFreeKernelHeap(ep);
            return OUT_OF_RESOURCES;
        }
        ep->qh->physical = qhPhysical;
        ep->qh->qh.qeLinkPointer = UHCI_LP_T;
        ep->qh->qh.qhLinkPointer = UHCI_LP_T;
    }

    if(USB_EP_INTERRUPT == config->type)
        ep->interval = stdc_bit_floor(config->period);

    if(config->maxDataSize < config->maxPacketSize)
    {
        LOG(SYSLOG_WARNING, "Dev %" PRIX8 ", EP %" PRIX8 ": max data size is less than max packet size", config->deviceAddress, config->endpointAddress);
        ep->config.maxDataSize = UHCI_DEFAULT_TRANSFER_SIZE;
    }
    else if(config->maxDataSize > UHCI_MAX_TRANSFER_SIZE)
    {
        LOG(SYSLOG_WARNING, "Dev %" PRIX8 ", EP %" PRIX8 ": max data size exceeds the hardcoded maximum", config->deviceAddress, config->endpointAddress);
        ep->config.maxDataSize = UHCI_MAX_TRANSFER_SIZE;
    }

    if(info->widePhysicalAddress)
    {
        size_t size = ALIGN_UP(ep->config.maxDataSize, PAGE_SIZE);
        void *buffer = MmReserveDynamicMemory(size);
        if(nullptr == buffer)
        {
            MmFreeKernelHeap(ep);
            return OUT_OF_RESOURCES;
        }

        status = MmAllocateMemoryFromPool(
            (uintptr_t)buffer, 
            size, 
            MM_FLAG_PRESENT | MM_FLAG_WRITABLE | MM_FLAG_WRITE_THROUGH | MM_FLAG_CACHE_DISABLE | MM_FLAG_NON_EXECUTABLE,
            I686_PHYSICAL_POOL_PCI_DMA);
        
        if(OK == status)
        {
            ep->buffer = MmBuildMemoryDescriptorList(buffer, size);
            if(nullptr == ep->buffer)
                status = OUT_OF_RESOURCES;
        }

        if(nullptr == ep->buffer)
        {
            if(OK == status)
                MmFreeMemory((uintptr_t)buffer, size);
            MmFreeDynamicMemoryReservation(buffer);
            MmFreeKernelHeap(ep);
            return status;
        }
    }

    *handle = ep;

    return OK;
}

static struct UhciTd* UhciAllocateTd(struct UhciControllerInfo *info)
{
    struct UhciTd *td = nullptr;
    PADDRESS physical = 0;

    td = MmSlabAllocateP(info->tdSlab, &physical);
    if(nullptr != td)
    {
        memset(td, 0, sizeof(*td));
        td->physical = physical;
    }
    return td;
}

static struct UhciTd *UhciAllocateTds(struct UhciControllerInfo *info, size_t count)
{
    struct UhciTd *td = nullptr;
    struct UhciTd *first = nullptr;
    struct UhciTd *last = nullptr;
    PADDRESS physical = 0;

    while(count--)
    {
        td = MmSlabAllocateP(info->tdSlab, &physical);
        if(nullptr == td)
        {
            while(nullptr != first)
            {
                td = first->next;
                MmSlabFreeP(info->tdSlab, first, first->physical);
                first = td;
            }
            return nullptr;
        }
        memset(td, 0, sizeof(*td));
        td->physical = physical;
        td->previous = last;
        if(nullptr != last)
            last->next = td;
        else
            first = td;
        last = td;
    }
    return first;
}

static void UhciFreeTd(struct UhciControllerInfo *info, struct UhciTd *td)
{
    MmSlabFree(info->tdSlab, td);
}

static void UhciFreeTds(struct UhciControllerInfo *info, struct UhciTd *first, struct UhciTd *last)
{
    do
    {
        struct UhciTd *t = first->next;
        MmSlabFree(info->tdSlab, first);
        first = t;
    }
    while((first != last) && (nullptr != first));
}

static bool UhciIsBufferOver32Bits(const struct MmMemoryDescriptor *memory)
{
    while(nullptr != memory)
    {
        if(memory->physical & ~((PADDRESS)UINT32_MAX))
            return true;
        memory = memory->next;
    }
    return false;
}

STATUS UhciStartTransfer(struct UsbHcd *hcd, struct UsbTransfer *transfer, struct UhciEndpoint *ep, struct UhciTransfer *context)
{
    STATUS status = OK;
    struct UhciControllerInfo *info = hcd->hcdData;
    size_t size = 0;
    const bool useBuffer = UhciIsBufferOver32Bits(transfer->data);
    struct UhciTd *first = nullptr;
    struct UhciTd *last = nullptr;
    bool dataOut = false;

    if(USB_EP_CONTROL == ep->config.type)
    {
        if(USB_XFER_OUT == transfer->direction)
            dataOut = true;
        else if(USB_XFER_IN != transfer->direction)
            return BAD_PARAMETER;
        //otherwise false be default
    }
    else
    {
        if(USB_XFER_OUT == ep->config.direction)
            dataOut = true;
        //otherwise false by default
    }

    memset(context, 0, sizeof(*context));
    
    size = MmGetMemoryDescriptorListSize(transfer->data);

    if(0 == size)
        first = UhciAllocateTd(info);
    else
    {
        size_t requiredTds = 0;

        if(!useBuffer)
        {
            struct MmMemoryDescriptor *memory = transfer->data;
            while(nullptr != memory)
            {
                requiredTds += CEIL_DIV(memory->size, ep->config.maxPacketSize);
                memory = memory->next;
            }
        }
        else
        {
            requiredTds = CEIL_DIV(size, ep->config.maxPacketSize);
        }

        first = UhciAllocateTds(info, requiredTds);
    }

    if(nullptr == first)
        return OUT_OF_RESOURCES;
    last = first;
    while(nullptr != last->next)
        last = last->next;

    if(dataOut && useBuffer && (0 != size))
    {
        void *b = HalMapMemoryDescriptorList(transfer->data);
        if(nullptr == b)
        {
            UhciFreeTds(info, first, last);
            return OUT_OF_RESOURCES;
        }
        memcpy(b, ep->buffer->mapped, size);
        HalUnmapMemoryDescriptorList(b);
    }
    
    struct MmMemoryDescriptor *memory = transfer->data;
    size_t remainingInBlock = (nullptr != memory) ? memory->size : 0;
    size_t remaining = size;
    bool toggle = false;
    struct UhciTd *td = first;
    do
    {
        td->td.token |= dataOut ? UHCI_TOKEN_PID_OUT : UHCI_TOKEN_PID_IN;
        td->td.token |= ((uint32_t)ep->config.deviceAddress << UHCI_TOKEN_ADDR_SHIFT) & UHCI_TOKEN_ADDR_MASK;
        td->td.token |= ((uint32_t)ep->config.endpointAddress << UHCI_TOKEN_ENDPT_SHIFT) & UHCI_TOKEN_ENDPT_MASK;
        td->td.token |= toggle ? UHCI_TOKEN_D : 0;
        if(remaining > ep->config.maxPacketSize)
        {
            td->td.token |= ((ep->config.maxPacketSize - 1) << UHCI_TOKEN_MAXLEN_SHIFT) & UHCI_TOKEN_MAXLEN_MASK;
            remaining -= ep->config.maxDataSize;
        }
        else
        {
            td->td.token |= ((remaining - 1) << UHCI_TOKEN_MAXLEN_SHIFT) & UHCI_TOKEN_MAXLEN_MASK;
            remaining = 0;
        }

        toggle = !toggle;

        td->td.controlAndStatus |= UHCI_CNTL_STATUS_ACTIVE | UHCI_CNTL_SPD;
        td->td.controlAndStatus |= (3 << UHCI_CNTL_ECNT_SHIFT);
        if(USB_EP_ISOCHRONOUS == ep->config.type)
            td->td.controlAndStatus |= UHCI_CNTL_IOS;
        if(USB_LOW_SPEED == ep->config.speed)
            td->td.controlAndStatus |= UHCI_CNTL_LS;

        if(USB_EP_BULK != ep->config.type)
            td->td.linkPointer |= UHCI_LP_VF;

        if(nullptr != td->next)
            td->td.linkPointer |= (td->next->physical & UHCI_LP_MASK);
        else
            td->td.linkPointer |= UHCI_LP_T;

        td = td->next;
    } 
    while(0 != remaining);
    
}