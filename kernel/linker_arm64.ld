OUTPUT_FORMAT("elf64-littleaarch64")
OUTPUT_ARCH(aarch64)
ENTRY(_start)

SECTIONS
{
    . = 0x40080000;

    .text ALIGN(4096) :
    {
        . = ALIGN(16);
        *(.text)
        *(.text.*)
        . = ALIGN(16);
    }

    .rodata ALIGN(4096) :
    {
        . = ALIGN(16);
        *(.rodata)
        *(.rodata.*)

        . = ALIGN(16);
        __kern_tests_start = .;
        KEEP(*(.kern_tests))
        __kern_tests_end = .;

        . = ALIGN(16);
        __subsys_tests_start = .;
        KEEP(*(.subsys_tests))
        __subsys_tests_end = .;

        . = ALIGN(16);
    }

    .data ALIGN(4096) :
    {
        . = ALIGN(16);
        *(.data)
        *(.data.*)
        . = ALIGN(16);
    }

    .bss ALIGN(65536) :
    {
        bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        bss_end = .;
    }

    .pstore ALIGN(4096) (NOLOAD) :
    {
        _pstore_start = .;
        *(.pstore)
        . = . + 4096; /* Reserve 4KB for crash logs */
        _pstore_end = .;
    }

    . = ALIGN(4096);
    _end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }
}
