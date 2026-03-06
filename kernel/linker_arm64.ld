OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(kernel_main)

SECTIONS
{
    . = 0x40080000;

    .text ALIGN(4096) :
    {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4096) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data ALIGN(4096) :
    {
        *(.data)
        *(.data.*)
    }

    .bss ALIGN(4096) :
    {
        bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        bss_end = .;
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
