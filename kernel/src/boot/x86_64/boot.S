/* boot.S — Multiboot2 x86_64 kernel entry
 *
 * Minimal bring-up: Multiboot2 header → long mode → kernel_main()
 *
 * Design: Everything is identity-mapped (virtual address == physical address)
 * at load time. The kernel is loaded at 0x100000 and the page tables live in
 * .bss, so all addresses here are plain physical addresses that the linker
 * resolves directly with R_X86_64_32 relocations (no higher-half arithmetic).
 */

/* ── Multiboot2 constants ──────────────────────────────────────────────── */
#define MB2_MAGIC    0xe85250d6
#define MB2_ARCH_X86 0

/* ── CPU flag bits ────────────────────────────────────────────────────────*/
#define CR4_PAE      (1 << 5)
#define CR0_PE       (1 << 0)
#define CR0_PG       (1 << 31)
#define EFER_MSR     0xC0000080
#define EFER_LME     (1 << 8)
#define PAGE_P       1
#define PAGE_RW      2
#define PAGE_PS      (1 << 7)   /* large (2 MiB) page */

/* ── Multiboot2 header ────────────────────────────────────────────────────
 * Must be 32-bit aligned and in the first 32 KiB of the image.
 */
.section .multiboot, "a"
.align 8
mb2_start:
    .long  MB2_MAGIC
    .long  MB2_ARCH_X86
    .long  (mb2_end - mb2_start)
    .long  -(MB2_MAGIC + MB2_ARCH_X86 + (mb2_end - mb2_start))
    /* end tag */
    .short 0
    .short 0
    .long  8
mb2_end:

/* ── Minimal bootstrap page tables (identity-map first 1 GiB) ────────────
 * All in .bss — zeroed by the boot code itself before use.
 */
.section .bss, "aw", @nobits
.align 4096
pml4: .skip 4096
pdpt: .skip 4096
pdt:  .skip 4096      /* 512 × 2 MiB = 1 GiB */

/* 16 KiB bootstrap stack */
.align 16
stack_bottom:
    .skip 16384
stack_top:

/* ── 32-bit protected-mode entry (GRUB / QEMU -kernel places us here) ────*/
.section .text
.code32
.global _start
_start:
    cli
    movl $stack_top, %esp   /* physical address — no arithmetic needed */

    /* Zero the three page-table pages */
    movl $pml4,  %edi
    xorl %eax,   %eax
    movl $(3*4096/4), %ecx
    rep stosl

    /* PML4[0] → PDPT (present + write) */
    movl $(pdpt + PAGE_P + PAGE_RW), %eax
    movl %eax, pml4

    /* PDPT[0] → PDT (present + write) */
    movl $(pdt + PAGE_P + PAGE_RW), %eax
    movl %eax, pdpt

    /* Fill PDT with 512 × 2 MiB identity pages */
    movl $512,  %ecx
    movl $pdt,  %edi
    movl $(PAGE_P + PAGE_RW + PAGE_PS), %eax
.fill_pdt:
    movl %eax, (%edi)
    movl $0, 4(%edi)
    addl $0x200000, %eax
    addl $8, %edi
    loop .fill_pdt

    /* Load CR3 */
    movl $pml4, %eax
    movl %eax,  %cr3

    /* Enable PAE */
    movl %cr4,  %eax
    orl  $CR4_PAE, %eax
    movl %eax,  %cr4

    /* Set EFER.LME */
    movl $EFER_MSR, %ecx
    rdmsr
    orl  $EFER_LME, %eax
    wrmsr

    /* Enable paging → activates long mode */
    movl %cr0,        %eax
    orl  $(CR0_PG | CR0_PE), %eax
    movl %eax,        %cr0

    /* Load a minimal GDT and far-jump into 64-bit code */
    lgdtl (gdt_ptr)
    ljmpl $0x08, $(_start64)

/* ── Minimal GDT (null + 64-bit code + 64-bit data) ─────────────────────*/
.section .rodata
.align 8
gdt:
    .quad 0x0000000000000000   /* null descriptor */
    .quad 0x00af9a000000ffff   /* 64-bit code: DPL0, exec/read */
    .quad 0x00af92000000ffff   /* 64-bit data: DPL0, read/write */
gdt_ptr:
    .short (gdt_ptr - gdt - 1)
    .long  gdt

/* ── 64-bit long mode entry ──────────────────────────────────────────────*/
.section .text
.code64
_start64:
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    xorw %ax, %ax
    movw %ax, %fs
    movw %ax, %gs

    /* Clear .bss (skip the page-table area which we already zeroed) */
    movq $bss_start, %rdi
    movq $bss_end,   %rcx
    subq %rdi, %rcx
    xorb %al,  %al
    rep stosb

    /* Call the C kernel — should never return */
    call kernel_main

.hang:
    hlt
    jmp .hang
