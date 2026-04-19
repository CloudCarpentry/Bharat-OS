import json
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
    protocol = "linux_arm64" if arch == "arm64" else "multiboot2" if arch == "x86_64" else "opensbi_payload"
    artifact_format = "raw_bin" if protocol == "linux_arm64" else "elf"

    transforms = []
    if artifact_format == "raw_bin":
        transforms.append(PackageTransformConfig(
            type="elf_to_bin",
            input="kernel/kernel.elf",
            output="kernel.bin"
        ))
        boot_artifact = "kernel.bin"
    else:
        boot_artifact = "kernel/kernel.elf"

    dtb_mode = "qemu_generated"

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
            boot_artifact=boot_artifact,
            nographic=nographic,
            serial=["stdio"],
        ),
        source_metadata={"source_kind": "legacy"}
    )
