//This header file is generated automatically
#ifndef EXPORTED_KERNEL_H_
#define EXPORTED_KERNEL_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "assert.h"
#include "color.h"
#include "common.h"
#include "config.h"
#include "defines.h"
#include "multiboot.h"
#include "common/order.h"
#include "common/vprintf.h"
#include "ddk/fs.h"
#include "ddk/stor.h"
#include "ex/elf.h"
#include "ex/exec.h"
#include "ex/ksym.h"
#include "ex/load.h"
#include "ex/kdrv/fsdrv.h"
#include "ex/kdrv/kdrv.h"
#include "hal/arch.h"
#include "hal/archdefs.h"
#include "hal/cpu.h"
#include "hal/hal.h"
#include "hal/interrupt.h"
#include "hal/math.h"
#include "hal/time.h"
#include "hal/i686/acpi.h"
#include "hal/i686/cpu.h"
#include "hal/i686/dcpuid.h"
#include "hal/i686/fpu.h"
#include "hal/i686/gdt.h"
#include "hal/i686/i686.h"
#include "hal/i686/ioapic.h"
#include "hal/i686/ioport.h"
#include "hal/i686/ipi.h"
#include "hal/i686/irq.h"
#include "hal/i686/lapic.h"
#include "hal/i686/math.h"
#include "hal/i686/memory.h"
#include "hal/i686/mp.h"
#include "hal/i686/msr.h"
#include "hal/i686/pic.h"
#include "hal/i686/pit.h"
#include "hal/i686/root.h"
#include "hal/i686/sse.h"
#include "hal/i686/time.h"
#include "hal/i686/tsc.h"
#include "hal/i686/bootvga/bootvga.h"
#include "hal/i686/bootvga/font.h"
#include "hal/i686/interrupts/exceptions.h"
#include "hal/i686/interrupts/it.h"
#include "io/initrd.h"
#include "io/dev/bus.h"
#include "io/dev/dev.h"
#include "io/dev/enumeration.h"
#include "io/dev/op.h"
#include "io/dev/res.h"
#include "io/dev/rp.h"
#include "io/dev/vol.h"
#include "io/disp/print.h"
#include "io/fs/devfs.h"
#include "io/fs/fs.h"
#include "io/fs/mount.h"
#include "io/fs/taskfs.h"
#include "io/fs/vfs.h"
#include "io/log/syslog.h"
#include "it/it.h"
#include "it/wrappers.h"
#include "ke/core/dpc.h"
#include "ke/core/mutex.h"
#include "ke/core/panic.h"
#include "ke/sched/idle.h"
#include "ke/sched/sched.h"
#include "ke/sched/sleep.h"
#include "ke/task/attach.h"
#include "ke/task/task.h"
#include "mm/avl.h"
#include "mm/dynmap.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "mm/mmio.h"
#include "mm/palloc.h"
#include "mm/slab.h"
#include "ob/ob.h"

#ifdef __cplusplus
}
#endif

#endif