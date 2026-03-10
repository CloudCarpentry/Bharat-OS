OUTPUT_FORMAT("elf64-littleriscv")
OUTPUT_ARCH(riscv)
ENTRY(_start)

SECTIONS
{
    . = 0x80200000;

    .text ALIGN(4096) :
    {
        *(.text.init)
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN(4096) :
    {
        *(.rodata)
        *(.rodata.*)

        . = ALIGN(8);
        __kern_tests_start = .;
        KEEP(*(.kern_tests))
        __kern_tests_end = .;

        . = ALIGN(8);
        __subsys_tests_start = .;
        KEEP(*(.subsys_tests))
        __subsys_tests_end = .;
    }

    .data ALIGN(4096) :
    {
        *(.data)
        *(.data.*)
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
