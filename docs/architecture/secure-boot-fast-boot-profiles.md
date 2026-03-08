# Secure Boot + Fast Boot Profiles (Automotive/RTOS/Robot/Drone)

## What this adds

Bharat-OS now selects an early boot policy that combines:
- secure-boot enforcement level,
- fast-boot mode (`balanced`, `fast`, `ultra-fast`), and
- bring-up knobs (timer rate, SMP core count, optional subsystems).

The policy is consumed in `kernel_main()` before major subsystem initialization.

## Policy matrix

- **RTOS hardware profile**: enforced secure boot, ultra-fast boot, high timer tick, single-core bring-up, skip zswap and AI governor.
- **EDGE hardware profile**: enforced secure boot, fast boot, 2-core bring-up, keep zswap, skip AI governor.
- **DRONE/ROBOT/AUTOMOTIVE_ECU/AUTOMOTIVE_INFOTAINMENT device profiles**: enforced secure boot, ultra-fast boot, high timer tick, skip non-critical boot-time features.
- **Default**: measured boot and balanced startup.

## Arch-specific secure-boot checks

The architecture HAL now contributes a secure-boot check:
- **x86_64**: enforced mode expects ACPI firmware profile to be selected.
- **arm64**: enforced mode expects FDT firmware profile.
- **riscv64**: enforced mode expects valid FDT pointer from boot firmware.

If enforced policy validation fails, kernel panics during early boot.

## QEMU / emulator testing (Windows 11 Pro)

### Prerequisites

- Install **MSYS2** or **WSL2 Ubuntu** on Windows 11 Pro.
- Install CMake + Ninja + cross toolchain.
- Install QEMU (`qemu-system-x86_64`, `qemu-system-aarch64`, `qemu-system-riscv64`).

### Example: x86_64 QEMU (measured boot)

```powershell
cmake -S . -B build-x86 -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/x86_64-elf.cmake `
  -DBHARAT_ARCH_FAMILY=X86 -DBHARAT_ARCH_BITS=64 `
  -DBHARAT_PLATFORM_FIRMWARE=ACPI `
  -DBHARAT_BOOT_HW_PROFILE=desktop
cmake --build build-x86
qemu-system-x86_64 -kernel build-x86/kernel/kernel.elf -serial stdio
```

### Example: ARM64 QEMU (automotive-like fast policy)

```powershell
cmake -S . -B build-arm -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/arm64-elf.cmake `
  -DBHARAT_ARCH_FAMILY=ARM -DBHARAT_ARCH_BITS=64 `
  -DBHARAT_DEVICE_PROFILE=AUTOMOTIVE_ECU `
  -DBHARAT_PLATFORM_FIRMWARE=FDT `
  -DBHARAT_BOOT_HW_PROFILE=edge
cmake --build build-arm
qemu-system-aarch64 -M virt -cpu cortex-a57 -nographic `
  -kernel build-arm/kernel/kernel.elf
```

### Example: RISC-V QEMU (RTOS ultra-fast policy)

```powershell
cmake -S . -B build-rv -G Ninja `
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv64-elf-gcc.cmake `
  -DBHARAT_ARCH_FAMILY=RISCV -DBHARAT_ARCH_BITS=64 `
  -DBHARAT_PLATFORM_FIRMWARE=FDT `
  -DBHARAT_BOOT_HW_PROFILE=rtos `
  -DBHARAT_DEVICE_PROFILE=DRONE
cmake --build build-rv
qemu-system-riscv64 -M virt -nographic -bios default `
  -kernel build-rv/kernel/kernel.elf
```

## Flash/run on real boards (generic production flow)

1. **Build with board profile + secure policy**
   - Set `BHARAT_DEVICE_PROFILE` to `AUTOMOTIVE_ECU`, `DRONE`, `ROBOT`, or `RTOS`.
   - Set platform firmware correctly (`ACPI` vs `FDT`).
2. **Produce board artifact**
   - ELF for JTAG load, or raw/payload image for boot ROM/U-Boot/OpenSBI.
3. **Sign artifact**
   - Sign with vendor boot key (OEM HSM/PKI).
   - Store cert chain/hash in board trust anchor (eFuse/TPM/secure element).
4. **Flash**
   - Typical tooling: `openocd + gdb`, `dfu-util`, `fastboot`, vendor flasher.
5. **Verify secure boot**
   - On serial log ensure `[SEC] Secure-boot policy accepted.` appears.
   - Ensure tampered image fails verification and does not continue boot.

For safety-critical targets (automotive, robotics, drones), keep production keys out of CI and use per-line provisioning.
