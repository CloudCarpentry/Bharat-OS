import sys
from pathlib import Path

from tools.build.models import ResolvedTarget


def validate_resolved_target(target: ResolvedTarget) -> None:
    validate_boot_contract(target)

    if target.kind == "qemu_target":
        validate_run_contract(target)
    elif target.kind == "board_target":
        validate_flash_contract(target)


def validate_boot_contract(target: ResolvedTarget) -> None:
    boot = target.boot
    kernel = target.kernel

    # If the boot contract requires raw bin, we must have an elf_to_bin transform OR kernel elf is somehow bin.
    # We check if there's an elf_to_bin transform in the package config if artifact_format is raw_bin.
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
