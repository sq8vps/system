OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
/* 
Below is the temporary kernel stack size that will be used only at the initialization stage.
Appropriate kernel CPU stack will be used after the first task switch.
*/
__KERNEL_INITIAL_STACK_SIZE = 4096; /* 4096 B */
__KERNEL_PAGE_SIZE = 4096; /* 4096 B */
__KERNEL_IMAGE_ADDRESS = 0xD6000000;
PROVIDE(_KERNEL_PAGE_SIZE = __KERNEL_PAGE_SIZE); 
SECTIONS
{
  . = __KERNEL_IMAGE_ADDRESS - __KERNEL_PAGE_SIZE; /* make some space for the multiboot header to be at the beginning of the file */
  PROVIDE(__istart = .);
  PROVIDE(_istart = .);
  PROVIDE(istart = .);
  .multiboot      :
  {
    *(.multiboot .multiboot.*)
  }
  . = __KERNEL_IMAGE_ADDRESS;
  PROVIDE(_kstart = .);
  PROVIDE(_stext = .);
  PROVIDE(_sboot = .);
  .boot           :
  {
    *(.boot .boot.*)
  }
  . = ALIGN(__KERNEL_PAGE_SIZE);
  .text           :
  {
    *(.text .text.*)
  }
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  . = ALIGN(__KERNEL_PAGE_SIZE);
  .rodata         :
  {
    *(.rodata .rodata.*)
  }
  . = ALIGN(__KERNEL_PAGE_SIZE);
  .data           :
  {
    *(.data .data.*);
    PROVIDE(_ctors = .);
    *(.ctors .ctors.*);
    PROVIDE(_ctors_end = .);
    *(.dtors .dtors.*);
  }
  . = ALIGN(__KERNEL_PAGE_SIZE);
  . = .;
  . = ALIGN(__KERNEL_PAGE_SIZE);
  __bss_start = .;
  .bss            :
  {
   *(.bss .bss.*)
   . = ALIGN(16);
   . += __KERNEL_INITIAL_STACK_SIZE;
   __KERNEL_INITIAL_STACK_ADDRESS = .;
   PROVIDE(_KERNEL_INITIAL_STACK_ADDRESS = .);
  }
  _end = .; 
  PROVIDE (end = .); 
  PROVIDE(_end = .);
  PROVIDE(_kend = .);
  PROVIDE(_iend = .);
}

