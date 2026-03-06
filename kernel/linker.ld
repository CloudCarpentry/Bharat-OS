/* linker.ld — Bharat-OS x86_64 Linker Script
 *
 * Lays out the kernel ELF to be loaded by a Multiboot2 bootloader (GRUB / QEMU -kernel).
 * The kernel is linked at physical 1 MiB (0x100000) — the traditional "high memory"
 * load address that avoids the real-mode BIOS interrupt table, video memory, etc.
 *
 * Virtual higher-half address is kept at the same physical address for now
 * (no higher-half relocation in this first bring-up; that is left for v2).
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* ── Load base ───────────────────────────────────────────────────────
     * 0x100000 = 1 MiB. This is where GRUB/QEMU -kernel will DMA the image.
     */
    . = 0x100000;

    /* ── Multiboot2 header — must appear within the first 32 KiB ─────── */
    .multiboot ALIGN(8) :
    {
        KEEP(*(.multiboot))
    }

    /* ── ELF Notes (PVH entry point info) ───────────────────────────── */
    .note ALIGN(4) :
    {
        KEEP(*(.note.*))
    }

    /* ── Read-only text ──────────────────────────────────────────────── */
    .text ALIGN(4096) :
    {
        *(.text)
        *(.text.*)
    }

    /* ── Read-only data ──────────────────────────────────────────────── */
    .rodata ALIGN(4096) :
    {
        *(.rodata)
        *(.rodata.*)
    }

    /* ── Initialized data ────────────────────────────────────────────── */
    .data ALIGN(4096) :
    {
        *(.data)
        *(.data.*)
    }

    /* ── Uninitialized data (zeroed by boot stub) ────────────────────── */
    .bss ALIGN(4096) :
    {
        bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        bss_end = .;
    }

    /* Discard DWARF / comment noise from the final binary */
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}
