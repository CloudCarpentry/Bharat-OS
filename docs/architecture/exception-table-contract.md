---
title: Bharat-OS Exception Table ABI Contract
status: Draft
owner: Documentation Working Group
last_updated: 2026-04-25
tags:
  - docs
  - architecture
see_also:
  - README.md
---
# Bharat-OS Exception Table ABI Contract

This document describes the canonical, unified Exception Table ABI contract across all Tier-1 architectures natively targeted by Bharat-OS (x86_64, ARM64, and RISC-V64).

---

## 1. Structure of an Exception Table Entry

Each exception table entry is exactly **12 bytes** in size, with standard 32-bit relative offsets and 16-bit type/data fields. This ensures consistent memory layout and strict alignment checking.

```c
struct bh_exception_entry {
    int32_t fault_off;  /* 32-bit relative offset to faulting instruction */
    int32_t fixup_off;  /* 32-bit relative offset to recovery instruction */
    uint16_t type;      /* 16-bit exception type (e.g. read vs write) */
    uint16_t data;      /* 16-bit auxiliary data */
};
```

### Offset Semantics
Both `fault_off` and `fixup_off` are computed relative to the address of the field itself:
- `fault_addr = (uintptr_t)&entry.fault_off + entry.fault_off;`
- `fixup_addr = (uintptr_t)&entry.fixup_off + entry.fixup_off;`

This relative offset design prevents absolute symbol relocation overhead and keeps exception entries compact and PIC/PIE-compatible.

---

## 2. Standardized Type Fields

The `type` field specifies the operation being performed at the faulting instruction:
- `BH_EX_UACCESS_RD` (Value: `2`) — User-space access read (e.g. `copy_from_user`).
- `BH_EX_UACCESS_WR` (Value: `3`) — User-space access write (e.g. `copy_to_user`).

---

## 3. Linker Script Integration

Every Tier-1 platform must expose the exception-table boundaries and align them precisely. Linker scripts must adhere to the following contract:

```ld
.rodata ALIGN(4096) :
{
    ...
    . = ALIGN(8);
    __ex_table_start = .;
    KEEP(*(__ex_table))
    __ex_table_end = .;
    ASSERT((__ex_table_end - __ex_table_start) % 12 == 0, "corrupt exception table");
    ...
}
```

The exception table (`__ex_table`) resides in read-only memory, while recovery fixups (`.fixup`) reside in the executable text section.

---

## 4. Platform Implementation Guidelines & Trap Protections

### Architectural Access Control & Fault Protection Matrix

| Architecture | Hardware Security Mechanism | Assembly Directive / Opcode | User Access Enable | Isolation Restore | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **x86_64** | **SMAP** (Supervisor Mode Access Prevention) | `stac` / `clac` | `stac` (Sets `EFLAGS.AC=1`) | `clac` (Clears `EFLAGS.AC=0`) | Operands use `%c` modifier for naked immediate constants in fixup table. |
| **ARM64** | **PAN** (Privileged Access Never) | `.arch_extension pan` | `msr pan, #0` (`PSTATE.PAN=0`) | `msr pan, %0` (`PSTATE.PAN=prev_pan`) | Assembly block MUST specify `.arch_extension pan` to prevent assembler errors under baseline `aarch64-unknown-none-elf` target flags. |
| **RISC-V64** | **SUM** (Supervisor User Memory) | `sstatus.SUM` bit | `csrs sstatus, %0` (Set `SUM=1`) | `csrc sstatus, %0` (Clear `SUM=0`) | Preserves `sstatus` across usercopy; faulting instruction branches via `.fixup`. |
| **ARM32** | Domain Access Control (DACR) / Boundary Check | `uaccess` boundary | Software range check | Software range check | Edge-tier MMU-lite configuration. |
| **RISC-V32** | PMP / Page Boundary Verification | `uaccess` boundary | Software range check | Software range check | Edge-tier MMU-lite configuration. |

### Critical Safety Invariants for Developers

1. **Do Not Remove PAN/SMAP/SUM Restoration:** Every usercopy function (`arch_copy_from_user_nofault`, `arch_copy_to_user_nofault`) MUST restore the original state before returning, even when an exception occurs (handled via `.fixup`).
2. **Assembler Extension Scope:** The `.arch_extension pan` directive is local to inline assembly blocks in `usercopy.c`. It permits opcode compilation without altering target CPU ABI or kernel trap frame layout.
3. **No Unhandled Traps:** All kernel-to-user copy operations MUST be wrapped in `__ex_table` entries. Unwrapped accesses to user pointers inside kernel space will cause unrecoverable page faults.

