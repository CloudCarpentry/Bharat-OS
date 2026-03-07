# Boot Flow: x86_64

## Overview

The x86_64 architecture is highly legacy-encumbered. Bharat-OS targets modern UEFI systems and relies on a Multiboot2-compliant bootloader (e.g., GRUB2 or Limine) to handle the initial switch to long mode.

## Sequence

```mermaid
sequenceDiagram
    participant UEFI as UEFI Firmware
    participant Bootloader as Bootloader (GRUB2/Limine)
    participant Stub as Assembly Stub (_start)
    participant Kernel as Microkernel (kernel_main)
    participant User as Root User Task (init)

    UEFI->>Bootloader: Load from ESP
    Bootloader->>Bootloader: Setup basic 64-bit page table
    Bootloader->>Bootloader: Collect Memory Map & Framebuffer
    Bootloader->>Stub: Jump to _start (with Multiboot2 magic)
    Stub->>Stub: Setup initial kernel stack
    Stub->>Kernel: Call kernel_main(magic, info_ptr)
    Kernel->>Kernel: Parse Multiboot2 tags (Physical Memory)
    Kernel->>Kernel: pmm_init() (Physical Memory Manager)
    Kernel->>Kernel: idt_init() (Interrupts)
    Kernel->>Kernel: vmm_init() (Virtual Memory Manager)
    Kernel->>User: Handover capabilities & Start
    User->>User: Spawn Personality Servers & Drivers
```

1. **UEFI Firmware**: Initializes the hardware, maps standard ACPI tables, and loads the bootloader from the ESP (EFI System Partition).
2. **Bootloader (GRUB2/Limine)**:
   - Reads the ELF64 Bharat-OS kernel binary.
   - Searches for the `MULTIBOOT2_HEADER_MAGIC` embedded in the kernel image.
   - Sets up a basic 64-bit page table mapping the first few MBs of memory.
   - Collects the Memory Map (E820/UEFI GetMemoryMap) and Framebuffer details.
   - Jumps to the kernel entry point (`_start` in assembly).
3. **Assembly Stub (`_start`)**:
   - Sets up the initial kernel stack.
   - Calls the C-level `kernel_main(magic, multiboot_info_ptr)`.
4. **Microkernel Initialization (`kernel_main`)**:
   - Parses the Multiboot2 tags to discover available physical memory.
   - Initializes the Physical Memory Manager (`pmm_init`).
   - Initializes the local APIC and core interrupt vectors (`idt_init`).
   - Re-maps the kernel into higher half virtual memory using the Virtual Memory Manager (`vmm_init`).
   - Starts the Idle Task and spawns the Root User Task (`init`).
5. **Root Task Handover**: The kernel gives the Root Task a capability encompassing all remaining physical memory, devices, and I/O ports. The Root Task then begins spawning Personality Servers and Drivers.
