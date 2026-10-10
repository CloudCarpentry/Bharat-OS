import json
import argparse
import sys
import struct
from pathlib import Path
from typing import Dict, Any, List

def parse_args():
    parser = argparse.ArgumentParser(description="Verify kernel artifact integrity against run manifest.")
    parser.add_argument("manifest", type=Path, help="Path to run-manifest.json")
    return parser.parse_args()

class ELFError(Exception):
    pass

def check_elf(path: Path) -> Dict[str, Any]:
    if not path.is_file():
        raise ELFError(f"File not found: {path}")

    size = path.stat().st_size
    if size == 0:
        raise ELFError("File is empty")

    if size < 52:
        raise ELFError("File too small to be an ELF")

    with open(path, 'rb') as f:
        e_ident = f.read(16)

        if e_ident[:4] != b'\x7fELF':
            raise ELFError("Invalid ELF magic number")

        ei_class = e_ident[4]
        if ei_class not in (1, 2):
            raise ELFError(f"Invalid ELF class: {ei_class}")

        ei_data = e_ident[5]
        if ei_data not in (1, 2):
            raise ELFError(f"Invalid ELF data encoding: {ei_data}")

        endian = '<' if ei_data == 1 else '>'

        if ei_class == 1:
            header = f.read(36)
            if len(header) < 36:
                raise ELFError("Truncated 32-bit ELF header")
            fields = struct.unpack(endian + 'HHIIIIIHHHHHH', header)
            e_machine, e_entry = fields[1], fields[3]
        else:
            header = f.read(48)
            if len(header) < 48:
                raise ELFError("Truncated 64-bit ELF header")
            fields = struct.unpack(endian + 'HHIQQQIHHHHHH', header)
            e_machine, e_entry = fields[1], fields[3]

        # e_machine: x86_64(62), ARM(40), AArch64(183), RISC-V32(243), RISC-V(243)
        if e_machine not in (62, 40, 183, 243):
            raise ELFError(f"Unsupported architecture machine code: {e_machine}")

        return {
            "class": ei_class,
            "machine": e_machine,
            "entry": e_entry
        }

def get_expected_elf_class_and_machine(arch: str) -> tuple[int, int]:
    # Returns (expected_class, expected_machine) based on architecture string
    # arch strings commonly used: x86_64, arm64, arm32, riscv32, riscv64
    mapping = {
        "x86_64": (2, 62),
        "arm64": (2, 183),
        "aarch64": (2, 183),
        "arm32": (1, 40),
        "arm": (1, 40),
        "riscv32": (1, 243),
        "riscv64": (2, 243)
    }
    return mapping.get(arch.lower(), (0, 0))

def verify_manifest(manifest_path: Path):
    if not manifest_path.is_file():
        print(f"Error: Manifest not found: {manifest_path}", file=sys.stderr)
        return False

    try:
        with open(manifest_path, 'r') as f:
            manifest = json.load(f)
    except Exception as e:
        print(f"Error reading manifest: {e}", file=sys.stderr)
        return False

    artifacts = manifest.get("artifacts", {})
    manifest_dir = manifest_path.parent
    expected_arch = manifest.get("arch", "")

    success = True

    # 1. Verify canonical elf
    canonical_elf_rel = artifacts.get("canonical_elf")
    if canonical_elf_rel:
        canonical_elf_path = (manifest_dir / canonical_elf_rel).resolve()
        try:
            print(f"Verifying canonical ELF: {canonical_elf_path}")
            elf_info = check_elf(canonical_elf_path)

            # Cross-reference with expected target architecture
            if expected_arch:
                exp_class, exp_machine = get_expected_elf_class_and_machine(expected_arch)
                if exp_class and elf_info['class'] != exp_class:
                    print(f"  Error: ELF class ({elf_info['class']}) does not match expected architecture '{expected_arch}' ({exp_class})", file=sys.stderr)
                    success = False
                if exp_machine and elf_info['machine'] != exp_machine:
                    print(f"  Error: ELF machine ({elf_info['machine']}) does not match expected architecture '{expected_arch}' ({exp_machine})", file=sys.stderr)
                    success = False

            if success:
                print(f"  Valid ELF (class={elf_info['class']}, machine={elf_info['machine']}, entry={elf_info['entry']:#x})")
        except ELFError as e:
            print(f"  Error: Canonical ELF check failed: {e}", file=sys.stderr)
            success = False
    else:
        print("  Error: Required canonical_elf missing in manifest", file=sys.stderr)
        success = False

    # 2. Verify referenced init-module
    init_module_rel = artifacts.get("init_module")
    if init_module_rel:
        init_module_path = (manifest_dir / init_module_rel).resolve()
        if init_module_path.is_file():
            print(f"Verified init-module exists: {init_module_path}")
        else:
            print(f"  Error: Missing init-module: {init_module_path}", file=sys.stderr)
            success = False

    # 3. Check dtb if required by boot contract
    boot_contract = manifest.get("boot_contract", {})
    dtb_req = boot_contract.get("dtb", {}).get("required", False)
    if dtb_req:
        dtb_rel = artifacts.get("dtb_path")
        if not dtb_rel:
            print("  Error: Boot contract requires dtb, but dtb_path is not in artifacts", file=sys.stderr)
            success = False
        else:
            dtb_path = (manifest_dir / dtb_rel).resolve()
            if not dtb_path.is_file():
                print(f"  Error: Required dtb not found: {dtb_path}", file=sys.stderr)
                success = False
            else:
                print(f"Verified dtb exists: {dtb_path}")

    return success

def main():
    args = parse_args()
    success = verify_manifest(args.manifest)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
