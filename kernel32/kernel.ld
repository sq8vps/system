OUTPUT_FORMAT("elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(main)
__KERNEL_STACK_SIZE = 0x20000; /* 128KiB */
__KERNEL_PAGE_SIZE = 4096; /* 4096 B*/
PROVIDE(_KERNEL_STACK_SIZE = __KERNEL_STACK_SIZE);
PROVIDE(_KERNEL_PAGE_SIZE = __KERNEL_PAGE_SIZE); 
SECTIONS
{
  /* This is the kernel image location. Specified in documentation. */
  . = 0xD0000000;
  .text           :
  {
    *(.text .text.*)
  }
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  . = ALIGN(__KERNEL_PAGE_SIZE);
  .data           :
  {
    *(.data .data.*)
  }
  . = .;
  __bss_start = .;
  .bss            :
  {
   *(.bss .bss.*)
   . = ALIGN(. != 0 ? 64 / 8 : 1);
   . += __KERNEL_STACK_SIZE;
   __KERNEL_STACK_ADDRESS = .;
   PROVIDE(_KERNEL_STACK_ADDRESS = .);
   . = ALIGN(. != 0 ? 64 / 8 : 1);
  }
  . = ALIGN(64 / 8);
  . = SEGMENT_START("ldata-segment", .);
  . = ALIGN(64 / 8);
  _end = .; PROVIDE (end = .);
}

