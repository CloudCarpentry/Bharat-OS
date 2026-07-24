# Core Arch Production Grade Review - 2026-04-25

## Files Reviewed
- `core/arch/x86/x86_64/boot/entry.c`
- `core/arch/arm/arm64/boot/entry.c`
- `core/arch/riscv/riscv64/boot/entry.c`
- `core/arch/x86/x86_64/boot/multiboot_parse.c`
- `core/hal/hal_pt.c`
- `core/hal/hal_mmu_caps.c`
- `core/arch/common/accel_caps_publish.c`
- `core/kernel/src/trap/trap.c`

## Issues Found

| ID | Severity | File | Issue | Production Risk | Status |
|---|---|---|---|---|---|
| 01 | P0 | `riscv64/boot/entry.c` | Hardcoded FDT fallback address (`0x8fe00000`). | Booting on incorrect DTB if loader fails to provide one. | Fixed |
| 02 | P1 | `core/hal/hal_pt.c` | Arch-specific `#ifdef` leakage for PT ops registration. | Violates 6-layer architecture and portability. | Fixed |
| 03 | P1 | `core/arch/common/accel_caps_publish.c` | Arch-specific `#ifdef` leakage in common code. | Maintenance burden and portability risk. | Fixed |
| 04 | P2 | `core/hal/hal_pt.c` | Weak alignment and flag validation in range operations. | Potential for kernel memory corruption or invalid PTEs. | Fixed |
| 05 | P2 | `x86_64/boot/entry.c` | Minimal validation of Multiboot structures after magic check. | Booting on corrupt bootloader data. | Fixed |
| 06 | P3 | `core/kernel/src/trap/trap.c` | Mixed arch-specific constants for syscall trap cause. | Minor cleanup needed for consistency. | Fixed |

## Build/Run Result Table

| Target | Build | QEMU headless | Boot marker | Notes |
|---|---:|---:|---|---|
| x86_64_desktop_headless | PASS | PASS | `Bharat-OS` | Verified hardened boot flow. |
| arm64_desktop_headless | PASS | PASS | `Bharat-OS` | Verified hardened boot flow. |
| riscv64_desktop_headless | PASS | PASS | `Bharat-OS` | Verified hardened boot flow. |
| arm32_mmu_lite_headless | PASS | FAIL | N/A | Build successful, but QEMU fails to produce output (Partial status). |
| riscv32_mmu_lite_headless | PASS | FAIL | N/A | Build successful, but QEMU fails due to missing OpenSBI binary. |

## Next Recommended Refactor Slice
- Unify Trap/Exception handling registration across architectures (similar to PT ops).
- Implement more robust CPU feature detection using the new `accel_caps` contract.
- Standardize IPI and TLB shootdown mechanisms to avoid unbounded waits.
