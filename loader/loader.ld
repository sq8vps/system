OUTPUT_FORMAT("binary")
/* OUTPUT_FORMAT("elf32-i386") */
OUTPUT_ARCH(i386)
ENTRY(loaderEntry)
SECTIONS
{
  . = 0xA000;
  .text           :
  {
    *(.ldr)
    *(.text .text.*)
  }
  PROVIDE (__etext = .);
  PROVIDE (_etext = .);
  PROVIDE (etext = .);
  . = ALIGN(4096);
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
  }
  . = ALIGN(64 / 8);
  . = SEGMENT_START("ldata-segment", .);
  . = ALIGN(64 / 8);
  _end = .; PROVIDE (end = .);
}

