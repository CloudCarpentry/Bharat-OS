import json
import subprocess
from shutil import which
from pathlib import Path

from tools.build.models import (
    BootConfig,
    BuildConfig,
    DebugConfig,
    DtbConfig,
    FlashConfig,
    KernelConfig,
    PackageConfig,
    PackageTransformConfig,
    ResolvedTarget,
    RunConfig,
)


def load_legacy_config(repo_root: Path) -> dict:
    manifest_path = repo_root / "build_config.json"
    if not manifest_path.exists():
        raise FileNotFoundError(f"Legacy config not found at {manifest_path}")

    with open(manifest_path, "r") as f:
        return json.load(f)


def resolve_objcopy_for_cmake() -> str | None:
    candidates = ("llvm-objcopy", "llvm-objcopy-20", "llvm-objcopy-19", "objcopy")
    for candidate in candidates:
        resolved = which(candidate)
        if not resolved:
            continue
        try:
            subprocess.run(
                [resolved, "--version"],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            return resolved
        except Exception:
            continue
    return None


def resolve_legacy_target(target_name: str, repo_root: Path) -> ResolvedTarget:
    config = load_legacy_config(repo_root)

    builds = config.get("builds", {})
    if target_name not in builds:
        raise ValueError(f"Legacy target '{target_name}' not found in build_config.json")

    legacy_cfg = builds[target_name]

    # Map legacy -> Normalized

    arch = legacy_cfg.get("arch", "unknown")
    board = legacy_cfg.get("board", "unknown")
    preset = legacy_cfg.get("preset", f"{arch}-edge")
    profile = legacy_cfg.get("profile", "unknown")
    personality = legacy_cfg.get("personality", "unknown")

    # Qemu defaults for legacy targets
    display_cfg = legacy_cfg.get("display", {})
    gui_enabled = display_cfg.get("enabled", legacy_cfg.get("gui", False))
    nographic = not gui_enabled

    # Fallback boot logic guessing
    protocol = "linux_arm64" if arch in ("arm64", "arm32") else "multiboot2" if arch == "x86_64" else "opensbi_payload"
    artifact_format = "raw_bin" if protocol == "linux_arm64" else "elf"

    transforms = []
    boot_artifact = "kernel/kernel.elf"

    if arch == "x86_64" and protocol == "multiboot2":
        transforms.append(PackageTransformConfig(
            type="multiboot_elf_fix",
            input="kernel/kernel.elf",
            output="kernel/kernel.elf32",
        ))
        boot_artifact = "kernel/kernel.elf32"

    if artifact_format == "raw_bin":
        transforms.append(PackageTransformConfig(
            type="elf_to_bin",
            input="kernel/kernel.elf",
            output="kernel/kernel.bin"
        ))
        boot_artifact = "kernel/kernel.bin"

    dtb_mode = "qemu_generated"
    default_cpu = "cortex-a72" if arch == "arm64" else None
    extra_args = legacy_cfg.get("extra_args", [])

    arch_family_map = {
        "x86_64": "X86_64",
        "x86": "X86",
        "arm64": "ARM64",
        "arm32": "ARM32",
        "riscv64": "RISCV64",
        "riscv32": "RISCV32",
    }

    cmake_defs = {
        "BHARAT_BOOT_GUI": "ON" if gui_enabled else "OFF",
        "BHARAT_ARCH_FAMILY": arch_family_map.get(str(arch).lower(), str(arch).upper()),
        "BHARAT_DEVICE_PROFILE": str(profile).upper(),
        "BHARAT_PERSONALITY_PROFILE": str(personality).upper(),
        "BHARAT_TARGET_BOARD": board,
    }
    cmake_defs.update(legacy_cfg.get("cmake_defs", {}))
    resolved_objcopy = resolve_objcopy_for_cmake()
    if resolved_objcopy:
        cmake_defs["CMAKE_OBJCOPY"] = resolved_objcopy

    return ResolvedTarget(
        name=target_name,
        kind="qemu_target",
        arch=arch,
        board=board,
        device_profile=profile,
        personality_profile=personality,
        execution_profile="gp",
        build=BuildConfig(
            cmake_preset=preset,
            cmake_defs=cmake_defs,
        ),
        kernel=KernelConfig(
            canonical_elf="kernel/kernel.elf",
        ),
        boot=BootConfig(
            protocol=protocol,
            artifact_format=artifact_format,
            dtb=DtbConfig(mode=dtb_mode, required=(arch == "arm64"))
        ),
        package=PackageConfig(
            transforms=transforms
        ),
        run=RunConfig(
            backend="qemu",
            machine="virt" if arch in ("arm64", "riscv64", "arm32", "riscv32") else "q35",
            cpu=default_cpu,
            boot_artifact=boot_artifact,
            nographic=nographic,
            serial=["stdio"],
            extra_args=extra_args,
        ),
        source_metadata={"source_kind": "legacy"}
    )
