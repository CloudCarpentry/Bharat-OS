import sys
from pathlib import Path

from tools.build.models import ResolvedTarget
from tools.build.footprint import validate_footprint_contract


def validate_resolved_target(target: ResolvedTarget, repo_root: Path | None = None) -> None:
    validate_boot_contract(target)
    validate_arch_profile_contract(target)
    if repo_root:
        try:
            validate_footprint_contract(target, repo_root)
        except ValueError as exc:
            print(f"Validation Error: {exc}")
            sys.exit(1)

    if target.kind == "qemu_target":
        validate_run_contract(target)
    elif target.kind == "board_target":
        validate_flash_contract(target)


def validate_boot_contract(target: ResolvedTarget) -> None:
    boot = target.boot
    kernel = target.kernel

    # Protocol-specific semantic validation
    if boot.protocol == "linux_arm64":
        # linux_arm64 requires a proper Linux Image header. 
        # A generic elf_to_bin (objcopy -O binary) is NOT sufficient as it lacks the header.
        is_generic_bin = any(t.type == "elf_to_bin" for t in target.package.transforms)
        if is_generic_bin:
            print(f"Validation Error: Protocol 'linux_arm64' on ARM64 cannot accept a generic 'elf_to_bin' output. "
                  f"It requires a real Linux ARM64 image with header. Use 'elf_direct' for QEMU simulation.")
            sys.exit(1)

    if boot.protocol == "elf_direct":
        if boot.artifact_format != "elf":
            print(f"Validation Error: Protocol 'elf_direct' requires artifact_format 'elf' for target '{target.name}'.")
            sys.exit(1)

    # General artifact format validation
    if boot.artifact_format == "raw_bin":
        has_transform = any(t.type == "elf_to_bin" for t in target.package.transforms)
        if not has_transform and not kernel.canonical_elf.endswith(".bin"):
            print(f"Validation Error: Boot contract requires 'raw_bin', but no 'elf_to_bin' transform is found for target '{target.name}'.")
            sys.exit(1)

    # Check DTB logic
    if boot.dtb.required and boot.dtb.mode not in ["qemu_generated", "external", "embedded", "appended", "firmware_provided"]:
        print(f"Validation Error: Invalid DTB mode '{boot.dtb.mode}' for target '{target.name}'.")
        sys.exit(1)


def validate_run_contract(target: ResolvedTarget) -> None:
    if not target.run:
        print(f"Validation Error: Target '{target.name}' is of kind 'qemu_target' but has no 'run' config.")
        sys.exit(1)

    run = target.run
    if run.backend == "qemu":
        if not run.machine:
            print(f"Validation Error: Run backend 'qemu' requires 'machine' config for target '{target.name}'.")
            sys.exit(1)

    if not run.boot_artifact:
        print(f"Validation Error: Missing 'boot_artifact' in run config for target '{target.name}'.")
        sys.exit(1)


def validate_flash_contract(target: ResolvedTarget) -> None:
    if not target.flash:
        print(f"Validation Error: Target '{target.name}' is of kind 'board_target' but has no 'flash' config.")
        sys.exit(1)

    flash = target.flash
    if flash.backend == "openocd" and not flash.artifact:
        print(f"Validation Error: Flash backend 'openocd' requires an 'artifact' to flash for target '{target.name}'.")
        sys.exit(1)


def validate_arch_profile_contract(target: ResolvedTarget) -> None:
    cmake_defs = target.build.cmake_defs or {}
    declared_family = str(cmake_defs.get("BHARAT_ARCH_FAMILY", "")).upper()
    expected_family = {
        "arm32": "ARM32",
        "arm64": "ARM64",
        "riscv32": "RISCV32",
        "riscv64": "RISCV64",
        "xtensa": "XTENSA",
        "arc": "ARC",
    }.get(target.arch, "")
    if expected_family and declared_family and declared_family != expected_family:
        print(
            f"Validation Error: Target '{target.name}' arch '{target.arch}' expects "
            f"BHARAT_ARCH_FAMILY={expected_family}, got '{declared_family}'."
        )
        sys.exit(1)
