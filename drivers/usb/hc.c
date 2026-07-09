#include "hc.h"
#include "logging.h"
#include "io/dev/bus.h"
#include "io/dev/dev.h"
#include "io/dev/res.h"
#include "mm/heap.h"
#include "hal/i686/ioport.h"
#include "ke/sched/sleep.h"
#include "hal/time.h"
#include "xfer.h"
#include "ex/worker.h"

#define UHCI_WORKER_POLLING_PERIOD 100 /**< UHCI worker thread polling period in ms */

#define PCI_UHCI_CLASS_CODE 0xC
#define PCI_UHCI_SUBCLASS_CODE 0x3

#define USBCMD 0x00
#define USBSTS 0x02
#define USBINTR 0x04
#define FRNUM 0x06
#define FLBASEADD 0x08
#define SOF 0x0C
#define PORTSC(port) (0x10 + (port) * 0x2)

#define USBCMD_RS 0x1
#define USBCMD_HCRESET 0x2
#define USBCMD_GRESET 0x4
#define USBCMD_EGSM 0x8
#define USBCMD_FGR 0x10
#define USBCMD_CF 0x40
#define USBCMD_MAXP 0x80

#define USBINTR_TIMEOUT 0x1
#define USBINTR_RESUME 0x2
#define USBINTR_IOC 0x4
#define USBINTR_SP 0x8

#define PORTSC_CONNECT_STATUS (1 << 0)
#define PORTSC_CONNECT_CHANGE (1 << 1)
#define PORTSC_ENABLE (1 << 2)
#define PORTSC_ENABLE_CHANGE (1 << 3)
#define PORTSC_LS (1 << 8)
#define PORTSC_RESET (1 << 9)
#define PORTSC_SUSPEND (1 << 12)

#define SOF_DEFAULT 64

#define LEGSUP 0xC0
#define LEGSUP_RESET 0x8F00
#define LEGSUP_DEFAULT 0x2000

static STATUS UhciIsr(void *context)
{
    return OK;
}

STATUS UhciResetHostController(struct UhciControllerInfo *info)
{
    IoPortWriteWord(info->ioPort + USBCMD, USBCMD_HCRESET);
    KeDelay(MS_TO_NS(1));
    if(IoPortReadWord(info->ioPort + USBCMD) & USBCMD_HCRESET)
    {
        LOG(SYSLOG_WARNING, "Host controller reset not completed after 1ms");
    }
    IoPortWriteWord(info->ioPort + USBINTR, 0);
    IoPortWriteWord(info->ioPort + USBCMD, 0);
    return OK;
}

STATUS UhciResetPort(struct UhciControllerInfo *info, size_t port)
{
    if(unlikely(port >= info->portCount))
        return BAD_PARAMETER;
    
    IoPortWriteWord(info->ioPort + PORTSC(port), PORTSC_RESET);
    KeDelay(MS_TO_NS(100)); //at least 50 ms for root ports reset
    IoPortWriteWord(info->ioPort + PORTSC(port), 0);
    KeDelay(MS_TO_NS(20)); //at least 10 ms for reset recovery
    return OK;
}

STATUS UhciEnablePort(struct UhciControllerInfo *info, size_t port, bool state)
{
    uint16_t status = 0;
    uint64_t timeout = 0;
    if(unlikely(port >= info->portCount))
        return BAD_PARAMETER;

    IoPortWriteWord(info->ioPort + PORTSC(port), state ? PORTSC_ENABLE : 0);
    timeout = HalGetTimestamp() + MS_TO_NS(100);
    while(1)
    {
        status = IoPortReadWord(info->ioPort + PORTSC(port));
        if(!!state == !!(status & PORTSC_ENABLE))
            break;
        if(HalGetTimestamp() > timeout)
            return TIMEOUT;
    }
    return OK;
}

static void UhciWorker(void *context)
{
    struct UhciControllerInfo *info = context;
    STATUS status = OK;
    while(1)
    {
        for(size_t i = 0; i < info->portCount; i++)
        {
            uint16_t t = IoPortReadWord(info->ioPort + PORTSC(i));
            if(t & PORTSC_CONNECT_CHANGE)
            {
                IoPortWriteWord(info->ioPort + PORTSC(i), t | PORTSC_CONNECT_CHANGE);
                if(t & PORTSC_CONNECT_STATUS)
                {
                    status = UhciResetPort(info, i);
                    if(OK == status)
                    {
                        status = UhciEnablePort(info, i, true);
                        
                    }
                }
                else
                {

                }
            }
        }
        KeSleep(MS_TO_NS(UHCI_WORKER_POLLING_PERIOD));
    }
}

STATUS UhciConfigureController(struct IoDeviceObject *bdo, struct IoDeviceObject *mdo, struct UhciControllerInfo *info)
{
    STATUS status = OK;
    struct IoPciDeviceHeader *hdr = nullptr;
    struct IoDeviceResource *resource = nullptr;
    uint32_t resourceCount = 0;
    struct IoIrqEntry *irq = nullptr;
    uint32_t irqInput = 0;

    info->dev = mdo;
    
    status = IoReadConfigSpace(bdo, 0, sizeof(*hdr), (void**)&hdr);

    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Getting PCI configuration failed with status 0x%X", status);
        return status;
    }

    if((PCI_UHCI_CLASS_CODE != hdr->classCode)
        || (PCI_UHCI_SUBCLASS_CODE != hdr->subclass)
        || (0 != hdr->progIf))
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

    uint16_t t = LEGSUP_RESET;
    status = IoWriteConfigSpace(bdo, LEGSUP, 2, &t);
    if(OK != status)
        return status;

    status = UhciResetHostController(info);
    if(OK != status)
        goto leave;

    hdr->command |= PCI_HEADER_COMMAND_IO_SPACE | PCI_HEADER_COMMAND_BUS_MASTER;

    status = IoWriteConfigSpace(bdo, offsetof(struct IoPciDeviceHeader, command), 1, &(hdr->command));
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Setting PCI configuration failed with status 0x%X", status);
        goto leave;
    }    


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

    IoPortWriteByte(info->ioPort + SOF, SOF_DEFAULT);
    IoPortWriteDWord(info->ioPort + FLBASEADD, (uint32_t)info->frameList);
    IoPortWriteWord(info->ioPort + FRNUM, 0);

    t = LEGSUP_DEFAULT;
    status = IoWriteConfigSpace(bdo, LEGSUP, 2, &t);
    if(OK != status)
        return status;

    IoPortWriteWord(info->ioPort + USBCMD, USBCMD_RS | USBCMD_CF | USBCMD_MAXP);
    IoPortWriteWord(info->ioPort + USBINTR, USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP);

    info->portCount = 0;
    while(1)
    {
        t = IoPortReadWord(info->ioPort + PORTSC(info->portCount));
        if(!(t & 0x80) || (0xFFFF == t))
            break;
        ++info->portCount;
    } 
    
    if(0 == info->portCount)
    {
        LOG(SYSLOG_ERROR, "No ports detected");
        goto leave;
    }

    status = ExCreateKernelWorker(UhciWorker, info, &info->worker);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Worker thread creation failed with status 0x%X", status);
        goto leave; 
    }
    
    LOG(SYSLOG_INFO, "Found %lu ports, initialization finished", info->portCount);

leave:
    MmFreeKernelHeap(hdr);
    MmFreeKernelHeap(resource);
    return status;
}