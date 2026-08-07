#include "hc.h"
#include "logging.h"
#include "io/dev/bus.h"
#include "io/dev/dev.h"
#include "io/dev/res.h"
#include "mm/heap.h"
#include "hal/mm.h"
#include "hal/i686/ioport.h"
#include "ke/sched/sleep.h"
#include "hal/time.h"
#include "xfer.h"
#include "ke/sched/sched.h"
#include "if.h"
#include "mm/palloc.h"

#define PCI_UHCI_CLASS_CODE 0xC
#define PCI_UHCI_SUBCLASS_CODE 0x3
#define PCI_UHCI_PROG_IF 0x0

#define USBCMD 0x00
#define USBSTS 0x02
#define USBINTR 0x04
#define FRNUM 0x06
#define FLBASEADD 0x08
#define SOFMOD 0x0C
#define PORTSC(port) (0x10 + (port) * 0x2)

#define USBCMD_RS 0x1
#define USBCMD_HCRESET 0x2
#define USBCMD_GRESET 0x4
#define USBCMD_EGSM 0x8
#define USBCMD_FGR 0x10
#define USBCMD_CF 0x40
#define USBCMD_MAXP 0x80

#define USBSTS_USBINT (1 << 0)
#define USBSTS_ERROR (1 << 1)
#define USBSTS_RESUME (1 << 2)
#define USBSTS_HSERROR (1 << 3)
#define USBSTS_HCPERROR (1 << 4)
#define USBSTS_HCHALTED (1 << 5)

#define USBINTR_TIMEOUT 0x1
#define USBINTR_RESUME 0x2
#define USBINTR_IOC 0x4
#define USBINTR_SP 0x8

#define PORTSC_CONNECT_STATUS (1 << 0)
#define PORTSC_CONNECT_CHANGE (1 << 1)
#define PORTSC_ENABLE (1 << 2)
#define PORTSC_ENABLE_CHANGE (1 << 3)
#define PORTSC_DPLUS (1 << 4)
#define PORTSC_DMINUS (1 << 5)
#define PORTSC_RESUME (1 << 6)
#define PORTSC_LS (1 << 8)
#define PORTSC_RESET (1 << 9)
#define PORTSC_SUSPEND (1 << 12)
#define PORTSC_RESERVED_MASK (0x7 << 13)

#define SOF_DEFAULT 64

#define LEGSUP 0xC0
#define LEGSUP_BIOS_MASK 0x8F00
#define LEGSUP_ENABLE_IRQ 0x2000

static STATUS UhciIsr(void *context)
{
    return OK;
}

static STATUS UhciInitializeRegisters(struct UsbHcd *hcd)
{
    STATUS status = OK;
    struct UhciControllerInfo *info = hcd->hcdData;

    uint8_t sof = IoPortReadByte(info->ioPort + SOFMOD);

    status = UhciResetHostController(hcd);
    if(OK != status)
        return status;
    
    IoPortWriteWord(info->ioPort + USBCMD, USBCMD_MAXP);
    IoPortWriteByte(info->ioPort + SOFMOD, sof);
    return OK;
}

static STATUS UhciTakeOverFromBios(struct UhciControllerInfo *info)
{
    STATUS status = OK;
    uint64_t timeout = 0;

    IoPortWriteWord(info->ioPort + USBCMD, 0);
    timeout = HalGetTimestamp() + MS_TO_NS(20);
    while(1)
    {
        if(IoPortReadWord(info->ioPort + USBSTS) & USBSTS_HCHALTED)
            break;
        if(HalGetTimestamp() > timeout)
        {
            LOG(SYSLOG_WARNING, "Timeout while waiting for the HC to halt");
            break;
        }
    }

    uint16_t legsup = LEGSUP_ENABLE_IRQ;
    status = IoWriteConfigSpace(info->bdo, LEGSUP, 2, &legsup);
    if(OK != status)
        LOG(SYSLOG_ERROR, "Failed to write LEGSUP, error 0x%X", (unsigned int)status);
    return status;
}

static STATUS UhciDetectPortCount(struct UhciControllerInfo *info)
{
    info->portCount = 0;
    while(1)
    {
        uint16_t t = IoPortReadWord(info->ioPort + PORTSC(info->portCount));
        if(!(t & 0x80) || (0xFFFF == t) || (127 == info->portCount))
            break;
        ++info->portCount;
    } 
    
    if(0 == info->portCount)
    {
        LOG(SYSLOG_ERROR, "No ports detected");
        return DEVICE_NOT_AVAILABLE;
    }

    return OK;
}

STATUS UhciResetHostController(struct UsbHcd *hcd)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    IoPortWriteWord(info->ioPort + USBCMD, USBCMD_GRESET);
    KeDelay(MS_TO_NS(20));
    IoPortWriteWord(info->ioPort + USBCMD, 0);
    return OK;
}

STATUS UhciResetPort(struct UsbHcd *hcd, size_t port)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    if(unlikely(port >= info->portCount))
        return BAD_PARAMETER;
    
    uint16_t t = (IoPortReadWord(info->ioPort + PORTSC(port)) & PORTSC_RESERVED_MASK) & ~(PORTSC_CONNECT_CHANGE | PORTSC_ENABLE_CHANGE);
    IoPortWriteWord(info->ioPort + PORTSC(port), t | PORTSC_RESET);
    KeDelay(MS_TO_NS(100)); //at least 50 ms for root ports reset
    IoPortWriteWord(info->ioPort + PORTSC(port), t & ~PORTSC_RESET);
    KeDelay(MS_TO_NS(20)); //at least 10 ms for reset recovery
    return OK;
}

STATUS UhciEnablePort(struct UsbHcd *hcd, size_t port, bool state)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    uint16_t t = 0;
    uint64_t timeout = 0;
    if(unlikely(port >= info->portCount))
        return BAD_PARAMETER;

    t = (IoPortReadWord(info->ioPort + PORTSC(port)) & PORTSC_RESERVED_MASK) & ~(PORTSC_CONNECT_CHANGE | PORTSC_ENABLE_CHANGE);
    if(state)
        t |= PORTSC_ENABLE;
    else
        t &= ~PORTSC_ENABLE;
    IoPortWriteWord(info->ioPort + PORTSC(port), t);
    timeout = HalGetTimestamp() + MS_TO_NS(100);
    while(1)
    {
        t = IoPortReadWord(info->ioPort + PORTSC(port));
        if(!!state == !!(t & PORTSC_ENABLE))
            break;
        if(HalGetTimestamp() > timeout)
            return TIMEOUT;
    }
    return OK;
}

STATUS UhciClearPortEnableChange(struct UsbHcd *hcd, size_t port)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    if(unlikely(port >= info->portCount))
        return BAD_PARAMETER;

    uint16_t t = (IoPortReadWord(info->ioPort + PORTSC(port)) & PORTSC_RESERVED_MASK) & ~(PORTSC_CONNECT_CHANGE | PORTSC_ENABLE_CHANGE);
    IoPortWriteWord(info->ioPort + PORTSC(port), t | PORTSC_ENABLE_CHANGE);
    return OK;
}

STATUS UhciClearPortConnectChange(struct UsbHcd *hcd, size_t port)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    if(unlikely(port >= info->portCount))
        return BAD_PARAMETER;

    uint16_t t = (IoPortReadWord(info->ioPort + PORTSC(port)) & PORTSC_RESERVED_MASK) & ~(PORTSC_CONNECT_CHANGE | PORTSC_ENABLE_CHANGE);
    IoPortWriteWord(info->ioPort + PORTSC(port), t | PORTSC_CONNECT_CHANGE);
    return OK;
}

STATUS UhciGetRootHubData(struct UsbHcd *hcd, struct UsbHcRootHubData *s)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    s->portCount = info->portCount;
    return OK;
}

STATUS UhciGetPortStatus(struct UsbHcd *hcd, size_t port, struct UsbRhPortStatus *s)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    uint16_t t = 0;
    if(unlikely(port >= info->portCount))
        return BAD_PARAMETER;    
    t = IoPortReadWord(info->ioPort + PORTSC(port));
    s->connect = !!(t & PORTSC_CONNECT_STATUS);
    s->connectChange = !!(t & PORTSC_CONNECT_CHANGE);
    s->enable = !!(t & PORTSC_ENABLE);
    s->enableChange = !!(t & PORTSC_ENABLE_CHANGE);
    s->lineStatus.plus = !!(t & PORTSC_DPLUS);
    s->lineStatus.minus = !!(t & PORTSC_DMINUS);
    s->resume = !!(t & PORTSC_RESUME);
    s->speed = (t & PORTSC_LS) ? USB_LOW_SPEED : USB_FULL_SPEED;
    s->reset = !!(t & PORTSC_RESET);
    s->resetChange = 0;
    s->suspdend = !!(t & PORTSC_SUSPEND);
    s->suspendChange = 0;
    return OK;
}

STATUS UhciStartHostController(struct UsbHcd *hcd)
{
    STATUS status = OK;
    struct UhciControllerInfo *info = hcd->hcdData;
    PADDRESS flPhysical = 0;

    status = UhciTakeOverFromBios(info);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Failed to take over from BIOS, error 0x%X", (unsigned int)status);
        return status;
    }

    status = UhciInitializeRegisters(hcd);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Failed to initialize host controller, error 0x%X", (unsigned int)status);
        return status;
    }

    status = HalGetPhysicalAddress((uintptr_t)(info->frameList), &flPhysical);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Failed to get physical frame list address, error 0x%X", (unsigned int)status);
        return status;
    }
    else if(unlikely(flPhysical & ~((PADDRESS)UINT32_MAX)))
    {
        LOG(SYSLOG_ERROR, "Frame list physical address is longer than 32 bits");
        return NOT_SUPPORTED;
    }

    IoPortWriteDWord(info->ioPort + FLBASEADD, flPhysical);
    IoPortWriteWord(info->ioPort + USBCMD, USBCMD_RS | USBCMD_CF | USBCMD_MAXP);
    IoPortWriteWord(info->ioPort + USBINTR, USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP);    
    info->controllerStarted = true;
    return OK;
}

STATUS UhciStopHostController(struct UsbHcd *hcd)
{
    struct UhciControllerInfo *info = hcd->hcdData;
    info->controllerStarted = false;
    IoPortWriteWord(info->ioPort + USBSTS, USBSTS_HCHALTED);
    IoPortWriteWord(info->ioPort + USBCMD, 0);
    IoPortWriteWord(info->ioPort + USBINTR, 0);    
    uint64_t timeout = HalGetTimestamp() + MS_TO_NS(100);
    uint16_t status = 0;
    while(1)
    {
        status = IoPortReadWord(info->ioPort + USBSTS);
        if(status & USBSTS_HCHALTED)
            break;
        if(HalGetTimestamp() > timeout)
            return TIMEOUT;
    }
    return OK;
}

STATUS UhciInitializeHostController(struct IoDeviceObject *bdo, struct IoDeviceObject *mdo, struct UsbHcd *hcd)
{
    STATUS status = OK;
    struct IoPciDeviceHeader *hdr = nullptr;
    struct IoDeviceResource *resource = nullptr;
    uint32_t resourceCount = 0;
    struct IoIrqEntry *irq = nullptr;
    uint32_t irqInput = 0;
    struct UhciControllerInfo *info = nullptr;

    hcd->hcdData = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(nullptr == hcd->hcdData)
    {
        LOG(SYSLOG_ERROR, "Unable to allocate memory");
        return OUT_OF_RESOURCES;
    }

    info = hcd->hcdData;

    info->dev = mdo;
    info->bdo = bdo;

    if(0 != (MmGetHighestUsablePhysicalMemory() & ~((PADDRESS)UINT32_MAX)))
        info->widePhysicalAddress = true;
    
    status = IoReadConfigSpace(info->bdo, 0, sizeof(*hdr), (void**)(&hdr));
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Getting PCI configuration failed with status 0x%X", status);
        return status;
    }

    if((PCI_UHCI_CLASS_CODE != hdr->classCode)
        || (PCI_UHCI_SUBCLASS_CODE != hdr->subclass)
        || (PCI_UHCI_PROG_IF != hdr->progIf))
    {
        LOG(SYSLOG_ERROR, "This device is not an UHCI controller");
        status = DEVICE_NOT_AVAILABLE;
        goto leave;
    }

    mdo->flags = IO_DEVICE_FLAG_ENUMERATION_CAPABLE;
    mdo->type = IO_DEVICE_TYPE_BUS;

    if(!(hdr->standard.bar[4] & 0x1))
    {
        LOG(SYSLOG_ERROR, "Expected BAR4 in UHCI to be IO port number, got MMIO");
        status = DEVICE_NOT_AVAILABLE;
        goto leave;
    }

    info->ioPort = hdr->standard.bar[4] & 0xFFE0;
    if(0 == info->ioPort)
    {
        LOG(SYSLOG_ERROR, "I/O base port in BAR4 is 0");
        status = DEVICE_NOT_AVAILABLE;
        goto leave;
    }

    hdr->command |= PCI_HEADER_COMMAND_IO_SPACE | PCI_HEADER_COMMAND_BUS_MASTER;

    status = IoWriteConfigSpace(bdo, offsetof(struct IoPciDeviceHeader, command), 1, &(hdr->command));
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Setting PCI configuration failed with status 0x%X", status);
        goto leave;
    }    

    status = UhciInitializeRegisters(hcd);
    if(OK != status)
        goto leave;

    status = IoGetDeviceResources(bdo, &resource, &resourceCount);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Getting bus configuration failed with status 0x%X", status);
        goto leave;
    }

    for(size_t i = 0; i < resourceCount; i++)
    {
        if(IO_RESOURCE_IRQ == resource[i].type)
        {
            irq = &(resource[i].irq);
            break;
        }
    }

    if((nullptr != irq) && (0 != irq->pin))
    {
        irqInput = irq->gsi;
        status = HalRegisterIrq(irqInput, UhciIsr, info, irq->params);
        if(OK == status)
            status = HalEnableIrq(irqInput, UhciIsr);
        if(OK != status)
        {
            LOG(SYSLOG_ERROR, "Setting up IRQ failed with status 0x%X", status);
            goto leave; 
        }
    }
    else
    {
        LOG(SYSLOG_ERROR, "Unknown IRQ for UHCI!");
        goto leave;
    }

    status = UhciAllocateFrameList(info);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Allocating frame list failed with status 0x%X", status);
        goto leave;
    }

    status = UhciDetectPortCount(info);
    if(OK == status)
        LOG(SYSLOG_INFO, "Found %lu ports, initialization finished", info->portCount);

leave:
    MmFreeKernelHeap(hdr);
    MmFreeKernelHeap(resource);
    return status;
}