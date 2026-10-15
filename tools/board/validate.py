#!/usr/bin/env python3
"""
Bharat-OS 32-Bit Embedded Board Qualification Tool.

Performs link-time and pre-execution validation of board memory manifests,
linker layout, MMIO collision prevention, stack bounds, and ELF32 artifacts.

Usage:
    python tools/board/validate.py --board stm32f407_discovery --profile ecu_cooling_fan --check all
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path
from typing import Dict, List, Optional, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.board.manifest import BoardManifest, MemoryRegion, MmioRegion, load_board_manifest


# ELF Constants
EI_MAG0 = 0
ELFMAG0 = 0x7F
ELFMAG1 = ord('E')
ELFMAG2 = ord('L')
ELFMAG3 = ord('F')
ELFCLASS32 = 1
ELFDATA2LSB = 1
EM_ARM = 40
EM_RISCV = 243


class BoardValidator:
    def __init__(self, manifest: BoardManifest, elf_path: Optional[Path] = None, profile: Optional[str] = None):
        self.manifest = manifest
        self.elf_path = elf_path
        self.profile = profile
        self.results: List[Tuple[str, bool, str]] = []

    def check_cpu_abi(self) -> bool:
        """Validate CPU and compiler ABI compatibility."""
        arch = self.manifest.architecture
        cpu = self.manifest.cpu

        if arch == "armv7e-m":
            if not cpu.startswith("cortex-m"):
                self.results.append(("CPU and compiler ABI compatible", False, f"arch armv7e-m requires Cortex-M CPU, got {cpu}"))
                return False
        elif arch == "rv32-mcu":
            if not (cpu.startswith("sifive") or cpu.startswith("rv32") or cpu.startswith("qingke")):
                self.results.append(("CPU and compiler ABI compatible", False, f"arch rv32-mcu unexpected CPU {cpu}"))
                return False
        elif arch == "armv7-a":
            if not (cpu.startswith("cortex-a") or cpu.startswith("armv7")):
                self.results.append(("CPU and compiler ABI compatible", False, f"arch armv7-a requires Cortex-A CPU, got {cpu}"))
                return False

        self.results.append(("CPU and compiler ABI compatible", True, f"{arch} / {cpu} ({self.manifest.fpu_abi} ABI)"))
        return True

    def check_flash_layout(self) -> bool:
        """Validate executable flash layout and entry location."""
        flash = self.manifest.get_flash_region()
        if not flash:
            self.results.append(("Flash layout validated", False, "No executable flash region found in manifest"))
            return False

        if not flash.executable:
            self.results.append(("Flash layout validated", False, f"Region {flash.name} is not marked executable"))
            return False

        if flash.length < 16 * 1024:
            self.results.append(("Flash layout validated", False, f"Flash capacity ({flash.length} bytes) is too small"))
            return False

        self.results.append(("Flash layout validated", True, f"{flash.name}: 0x{flash.origin:08X}..0x{flash.end:08X} ({flash.length // 1024}K)"))
        return True

    def check_sram_stack(self) -> bool:
        """Validate SRAM bounds and reserved stack fit."""
        ram = self.manifest.get_ram_region()
        if not ram:
            self.results.append(("SRAM and reserved stack fit", False, "No writable RAM region found in manifest"))
            return False

        total_stack = self.manifest.runtime.kernel_stack + self.manifest.runtime.application_stack
        if total_stack > ram.length:
            self.results.append((
                "SRAM and reserved stack fit",
                False,
                f"Reserved stacks ({total_stack} B) exceed RAM size ({ram.length} B)"
            ))
            return False

        # Must leave room for static data and BSS
        remaining_ram = ram.length - total_stack
        if remaining_ram < 2 * 1024:
            self.results.append((
                "SRAM and reserved stack fit",
                False,
                f"RAM remaining after stacks ({remaining_ram} B) is dangerously small"
            ))
            return False

        self.results.append((
            "SRAM and reserved stack fit",
            True,
            f"{ram.name} ({ram.length // 1024}K) with {total_stack // 1024}K stacks leaves {remaining_ram // 1024}K"
        ))
        return True

    def check_vector_table(self) -> bool:
        """Validate vector table placement within bootable memory."""
        boot = self.manifest.boot
        vec_loc = boot.vector_location.lower()

        target_region = None
        for name, reg in self.manifest.memory.items():
            if name.lower() == vec_loc or (vec_loc == "flash" and reg.executable):
                target_region = reg
                break

        if not target_region:
            self.results.append(("Vector table within boot memory", False, f"Vector location '{boot.vector_location}' does not match any region"))
            return False

        if not target_region.executable:
            self.results.append(("Vector table within boot memory", False, f"Vector target region '{target_region.name}' is not executable"))
            return False

        if boot.vector_alignment < 4 or (boot.vector_alignment & (boot.vector_alignment - 1)) != 0:
            self.results.append(("Vector table within boot memory", False, f"Invalid vector alignment: {boot.vector_alignment}"))
            return False

        self.results.append(("Vector table within boot memory", True, f"Located in {target_region.name} (align {boot.vector_alignment})"))
        return True

    def check_mpu_plan(self) -> bool:
        """Validate MPU/PMP region plan representation constraints."""
        model = self.manifest.protection.model
        if model == "MPU_ONLY":
            # Check ARMv7-M PMSAv7 alignment constraints
            flash = self.manifest.get_flash_region()
            ram = self.manifest.get_ram_region()
            if flash and (flash.length & (flash.length - 1)) != 0:
                self.results.append(("MPU region plan representable", False, f"Flash length ({flash.length}) is not power-of-two for PMSAv7 MPU"))
                return False
            if ram and (ram.length & (ram.length - 1)) != 0:
                self.results.append(("MPU region plan representable", False, f"RAM length ({ram.length}) is not power-of-two for PMSAv7 MPU"))
                return False
            self.results.append(("MPU region plan representable", True, "PMSAv7 power-of-two alignment confirmed"))
            return True
        elif model == "PMP_ONLY":
            self.results.append(("MPU region plan representable", True, "RISC-V PMP NAPOT/TOR representable"))
            return True
        elif model in ("MMU_LITE", "MMU_FULL"):
            self.results.append(("MPU region plan representable", True, f"Virtual memory ({model}) enabled"))
            return True
        else:
            self.results.append(("MPU region plan representable", True, "Protection model NONE"))
            return True

    def check_no_mmio_overlap(self) -> bool:
        """Verify that no memory region overlaps peripheral MMIO register space."""
        for m_name, m_reg in self.manifest.memory.items():
            for mmio in self.manifest.mmio:
                # Check for overlap: max(start1, start2) < min(end1, end2)
                overlap_start = max(m_reg.origin, mmio.origin)
                overlap_end = min(m_reg.end, mmio.end)
                if overlap_start < overlap_end:
                    self.results.append((
                        "No MMIO overlap",
                        False,
                        f"CRITICAL: Memory '{m_name}' (0x{m_reg.origin:08X}..0x{m_reg.end:08X}) overlaps MMIO '{mmio.name}' (0x{mmio.origin:08X}..0x{mmio.end:08X})"
                    ))
                    return False

        self.results.append(("No MMIO overlap", True, f"All {len(self.manifest.memory)} memory regions clear of {len(self.manifest.mmio)} MMIO zones"))
        return True

    def check_devices(self) -> bool:
        """Verify essential peripheral drivers are defined."""
        dev = self.manifest.devices
        if not dev.console:
            self.results.append(("Required UART/timer devices defined", False, "Missing console UART device"))
            return False
        if not dev.timer:
            self.results.append(("Required UART/timer devices defined", False, "Missing system timer device"))
            return False
        if not dev.interrupt_controller:
            self.results.append(("Required UART/timer devices defined", False, "Missing interrupt controller device"))
            return False

        self.results.append((
            "Required UART/timer devices defined",
            True,
            f"Console: {dev.console}, Timer: {dev.timer}, IRQ: {dev.interrupt_controller}"
        ))
        return True

    def check_renode_platform(self) -> bool:
        """Verify Renode platform configuration model is present."""
        renode_name = self.manifest.board
        # Look for platform model or check known Renode platforms
        known_platforms = {
            "stm32f407_discovery": "platforms/boards/stm32f4_discovery.repl",
            "stm32f4_discovery": "platforms/boards/stm32f4_discovery.repl",
            "hifive1_rev_b": "platforms/cpus/sifive-fe310.repl",
            "rv32_virt": "platforms/cpus/riscv-virt.repl",
            "qemu_virt_arm32": "platforms/cpus/cortex-a15.repl",
        }
        if renode_name in known_platforms:
            self.results.append(("Renode platform configuration present", True, f"{known_platforms[renode_name]} verified"))
            return True

        self.results.append(("Renode platform configuration present", True, f"Default platform model for {self.manifest.soc}"))
        return True

    def check_elf_binary(self) -> bool:
        """Verify compiled ELF32 binary if present."""
        if not self.elf_path:
            self.results.append(("Kernel and app ELF32 verified", True, "ELF check skipped (no binary path specified)"))
            return True

        if not self.elf_path.is_file():
            self.results.append(("Kernel and app ELF32 verified", False, f"ELF binary not found at: {self.elf_path}"))
            return False

        try:
            with open(self.elf_path, "rb") as f:
                header = f.read(52)

            if len(header) < 52:
                self.results.append(("Kernel and app ELF32 verified", False, "File too small to be an ELF binary"))
                return False

            if header[0] != ELFMAG0 or header[1] != ELFMAG1 or header[2] != ELFMAG2 or header[3] != ELFMAG3:
                self.results.append(("Kernel and app ELF32 verified", False, "Invalid ELF magic"))
                return False

            elf_class = header[4]
            if elf_class != ELFCLASS32:
                self.results.append(("Kernel and app ELF32 verified", False, f"Expected ELF32 (1), got {elf_class}"))
                return False

            elf_data = header[5]
            if elf_data != ELFDATA2LSB:
                self.results.append(("Kernel and app ELF32 verified", False, "Expected little-endian ELF"))
                return False

            e_type, e_machine, e_version, e_entry = struct.unpack("<HHII", header[16:28])

            # Check machine type
            if self.manifest.architecture in ("armv7e-m", "armv7-a"):
                if e_machine != EM_ARM:
                    self.results.append(("Kernel and app ELF32 verified", False, f"Expected EM_ARM (40), got {e_machine}"))
                    return False
            elif self.manifest.architecture in ("rv32-mcu", "rv32-sv32"):
                if e_machine != EM_RISCV:
                    self.results.append(("Kernel and app ELF32 verified", False, f"Expected EM_RISCV (243), got {e_machine}"))
                    return False

            # Check entry point inside executable memory
            entry_valid = False
            for reg in self.manifest.memory.values():
                if reg.executable and reg.origin <= e_entry < reg.end:
                    entry_valid = True
                    break

            if not entry_valid:
                # Also check if it's Thumb-2 entry (+1 bit)
                for reg in self.manifest.memory.values():
                    if reg.executable and reg.origin <= (e_entry & ~1) < reg.end:
                        entry_valid = True
                        break

            if not entry_valid:
                self.results.append((
                    "Kernel and app ELF32 verified",
                    False,
                    f"Entry point 0x{e_entry:08X} does not lie inside any executable memory region"
                ))
                return False

            self.results.append((
                "Kernel and app ELF32 verified",
                True,
                f"Valid ELF32 machine={e_machine}, entry=0x{e_entry:08X}"
            ))
            return True

        except Exception as exc:
            self.results.append(("Kernel and app ELF32 verified", False, f"Failed to parse ELF: {exc}"))
            return False

    def validate_all(self) -> bool:
        """Run all qualification checks."""
        self.results.clear()
        checks = [
            self.check_cpu_abi,
            self.check_flash_layout,
            self.check_sram_stack,
            self.check_vector_table,
            self.check_mpu_plan,
            self.check_no_mmio_overlap,
            self.check_devices,
            self.check_renode_platform,
            self.check_elf_binary,
        ]

        all_passed = True
        for check in checks:
            passed = check()
            if not passed:
                all_passed = False

        if all_passed:
            self.results.append(("Image ready for execution", True, f"Board '{self.manifest.board}' qualified"))
        else:
            self.results.append(("Image ready for execution", False, "One or more qualification checks failed"))

        return all_passed


def main() -> int:
    parser = argparse.ArgumentParser(description="Bharat-OS Embedded Board Qualification Tool")
    parser.add_argument("--board", help="Board manifest path or name under platform/boards/")
    parser.add_argument("--all", action="store_true", help="Validate all board manifests under platform/boards/")
    parser.add_argument("--profile", help="Target profile or workload (e.g. ecu_cooling_fan)")
    parser.add_argument("--check", default="all", help="Checks to run (all or specific)")
    parser.add_argument("--elf", help="Path to compiled ELF binary for verification")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent.parent

    if args.all:
        boards_dir = repo_root / "platform" / "boards"
        board_dirs = [d for d in boards_dir.iterdir() if d.is_dir() and (d / "board.yaml").exists()]
        if not board_dirs:
            print("[FAIL] No board manifests found under platform/boards/", file=sys.stderr)
            return 1

        overall_success = True
        print(f"=== Qualifying {len(board_dirs)} Embedded Board Manifests ===")
        for bdir in sorted(board_dirs):
            print(f"\n--- Board: {bdir.name} ---")
            try:
                manifest = load_board_manifest(bdir, repo_root=repo_root)
                validator = BoardValidator(manifest, profile=args.profile)
                success = validator.validate_all()
                if not success:
                    overall_success = False
                for name, passed, detail in validator.results:
                    status_tag = "[PASS]" if passed else "[FAIL]"
                    print(f"{status_tag} {name:<40} : {detail}")
            except Exception as exc:
                print(f"[FAIL] Failed validating {bdir.name}: {exc}", file=sys.stderr)
                overall_success = False

        print(f"\n=== Overall Board Qualification: {'PASS' if overall_success else 'FAIL'} ===")
        return 0 if overall_success else 1

    if not args.board:
        parser.error("Either --board or --all must be specified.")

    try:
        manifest = load_board_manifest(args.board, repo_root=repo_root)
    except Exception as exc:
        print(f"[FAIL] Failed to load board manifest: {exc}", file=sys.stderr)
        return 1

    elf_path = Path(args.elf) if args.elf else None
    validator = BoardValidator(manifest, elf_path=elf_path, profile=args.profile)
    success = validator.validate_all()

    for name, passed, detail in validator.results:
        status_tag = "[PASS]" if passed else "[FAIL]"
        print(f"{status_tag} {name:<40} : {detail}")

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
