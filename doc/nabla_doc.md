# Nabla kernel documentation
This is the documentation of concepts and their implementations in Nabla operating system kernel.

## What is Nabla?
Nabla aims to be a complete, general purpose, reasonably-lightweight hobby operating system containing:
* a highly modular monolithic kernel with kernel API,
* kernel-mode drivers (modules) for generic/abstract/universal devices, such as:
    * disk drives, terminals, audio, etc.,
    * ACPI, IDE, AHCI, USB, VGA, etc.,
    * typical filesystems: FAT, EXT, etc.
* kernel-mode drivers (modules) for popular/chosen devices,
* a user-space standard library,
* fundamental user-space programs and utilities.

One of the main goals, as opposed to many OSes, is to provide a consistent and uniform API for all kernel-mode drivers (or other modules), allowing for full code separation between the driver code and the kernel code. In other words, the kernel must not be aware of any driver internals and communicate with drivers using the abstract interface, e.g., the kernel is not interested in what the PCI or USB bus is.

Nabla is written purely for fun and educational purposes.

## What Nabla is not?
Nabla, as almost all other hobby operating systems, is not meant to be a replacement for a big-scale OS, such as GNU/Linux or Windows. Nabla is not meant to be compatible with Unix/Windows.

## Kernel API documentation
Kernel API is documented using Doxygen format and is available through the Doxygen-generated HTML documentation.  
If you are interested in how kernel-mode drivers interact with the kernel, keep reading.

## Kernel source code organization
Kernel source code is divided between multiple directories, effectively representing different kernel parts.  
The top-level directory organization is as follows:
* `ex` - executables and kernel-mode drivers handling,
* `hal` - hardware abstraction layers for each supported architecture,
* `io` - all kinds of input/output - devices, virtual file system, 
* `it` - high-level interrupt handling,
* `ke` - kernel core - task and concurrency handling, scheduler,
* `mm` - memory managament - including heap allocation, MMIO, dynamic memory, etc.,
* `ob` - kernel object handling,
* `templates` - C++ templates to abstract commonly used elements,
* `rtl` - kernel run-time library.

Each of these directories may contain multiple subdirectories dedicated for more specific kernel parts.  

## Naming conventions
In general, every kernel object and function name start with "module prefix", and these prefixes are, in most cases, the same as the top-level directory names. For example, functions starting with `Ke`, such as `KeAcquireSpinlock()`, are defined in some file within the `ke` directory and are core kernel routines. Some functions, such as the Multiboot handling routines, are not prefixed. The non-prefixed functions are generally excluded from API and are not used by kernel-mode drivers.  
The architecture-specific routines and object are prefixed by the architecture-specific prefix, e.g., for i686 the prefix is `I686`. These functions might be included in API.

## Kernel-mode API and driver linking
The functions and objects to be exported in API are marked with `EXPORT_API` and `END_EXPORT_API` macros. A small script written in Python allows for automatic generation of kernel API headers. Then, a kernel-mode driver includes the appropriate header to use the API.  
Kernel-mode drivers (or other modules) are compiled as relocatable executables, so their references to kernel routines and objects are not resolved by the linker. When the driver is loaded, the kernel selects the load address dynamically and links the executable dynamically (in other words, the kernel executable works as a dynamic library).  

## Boot requirements
### The bootloader
Nabla kernel is Multiboot2-compliant and requires a Multiboot2-compliant bootloader, such as GRUB. The kernel passes the following tags to the bootloader:
* header tag,
* relocatable header tag (type 10) - image must be loaded between physical address 0 and `MM_KERNEL_ADDRESS`,
* information request tag:
    * command line string (type 1),
    * basic memory (type 4) (amount of memory),
    * memory map (type 6),
    * modules (type 3).

The kernel expects the bootloader to provide *all* required tags and additionally:
* ELF-Symbols tag (type 9) with kernel symbols.

### Initial ramdisk
Nabla kernel expects one module to be reported and loaded by the bootloader, which is the initial ramdisk, with the name `initrd.tar`. The initial ramdisk is a uncompressed tar file containing basic system configuration databases and drivers required to read and write from the system partition. This includes all drivers required to build the complete disk drive stack and the volume stack (more on device/driver/volume stacks in the later sections).

## Kernel memory management
### Memory space layout
The exact kernel memory layout is architecture-dependent. However, the kernel memory space consists of a few fundamental regions:
* kernel image - the loaded kernel executable,
* heap - dynamically allocated memory mapping,
* driver space - loaded kernel-mode drivers' executables,
* dynamic memory - dynamically mapped memory without explicit allocation,
* paging structures - architecture-specific paging structures mappings,
* thread kernel stacks - kernel-mode stacks for each thread in a process.

### Physical memory allocation
The physical memory allocation is based on a buddy allocation algorithm. The smallest allocation unit size is defined by the `MM_BUDDY_SMALLEST` macro. The number of "layers" is defined by the `MM_BUDDY_COUNT` macro. Thus, the smallest allocatable block has a size of `MM_BUDDY_SMALLEST`, while the biggest allocatable block has a size of `MM_BUDDY_SMALLEST * (1 << MM_BUDDY_COUNT)`. The available physical memory is determined by the architecture-specific HAL based on memory map passed by the bootloader. The buddy allocation algorithm will not be described here.  
Nabla allows for multiple physical memory pools to be defined by the HAL. This might be required in some applications, e.g., in AMD64 systems, where the physical memory for PCI DMA transfer must lay below 4 GiB limit, while the actual available physical memory might be bigger. 