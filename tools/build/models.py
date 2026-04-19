from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional


@dataclass(frozen=True)
class TargetInput:
    source_kind: str  # "yaml" | "legacy"
    source_ref: str   # file path or legacy target name


@dataclass
class ArtifactRecord:
    kind: str                 # kernel_elf, raw_bin, run_manifest, etc.
    path: Path
    producer: str             # build_executor, elf_to_bin, runner_qemu, etc.
    description: str = ""
    metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class BuildConfig:
    cmake_preset: str
    cmake_defs: Dict[str, str] = field(default_factory=dict)
    build_dir: Optional[Path] = None
    configure_only: bool = False


@dataclass
class KernelConfig:
    canonical_elf: str
    canonical_map: Optional[str] = None
    canonical_symbols: Optional[str] = None
    entry_contract: Optional[str] = None


@dataclass
class DtbConfig:
    mode: str                 # qemu_generated | external | embedded | appended | firmware_provided
    required: bool = False
    path: Optional[str] = None
    handoff_register: Optional[str] = None


@dataclass
class BootConfig:
    protocol: str             # linux_arm64 | multiboot2 | opensbi_payload | raw_entry | uefi | ...
    artifact_format: str      # elf | raw_bin | boot_image | efi | flash_image
    dtb: DtbConfig


@dataclass
class PackageTransformConfig:
    type: str                 # elf_to_bin, elf_to_img, ...
    input: str
    output: str
    options: Dict[str, Any] = field(default_factory=dict)


@dataclass
class PackageConfig:
    transforms: List[PackageTransformConfig] = field(default_factory=list)


@dataclass
class RunConfig:
    backend: str              # qemu | renode | none
    machine: Optional[str] = None
    cpu: Optional[str] = None
    memory: Optional[str] = None
    boot_artifact: Optional[str] = None
    serial: List[str] = field(default_factory=list)
    display: Optional[str] = None
    nographic: bool = False
    extra_args: List[str] = field(default_factory=list)


@dataclass
class FlashConfig:
    backend: str              # openocd | pyocd | fastboot | none
    artifact: Optional[str] = None
    probe: Optional[str] = None
    transport: Optional[str] = None
    verify: bool = False
    reset_after_flash: bool = False
    extra_args: List[str] = field(default_factory=list)


@dataclass
class DebugConfig:
    backend: str              # gdb_remote | lldb_remote | none
    symbols: Optional[str] = None
    gdb_port: Optional[int] = None
    stop_on_entry: bool = False
    extra_args: List[str] = field(default_factory=list)


@dataclass
class ResolvedTarget:
    name: str
    kind: str                 # qemu_target | board_target
    arch: str
    board: str
    device_profile: str
    personality_profile: str
    execution_profile: Optional[str]

    build: BuildConfig
    kernel: KernelConfig
    boot: BootConfig
    package: PackageConfig
    run: Optional[RunConfig] = None
    flash: Optional[FlashConfig] = None
    debug: Optional[DebugConfig] = None
    footprint_profile: Optional[str] = None

    output_root: Optional[Path] = None
    source_metadata: Dict[str, Any] = field(default_factory=dict)


@dataclass
class BuildPlan:
    target: ResolvedTarget
    build_dir: Path
    manifest_path: Path
    command_summary: List[List[str]] = field(default_factory=list)


@dataclass
class BuildOutputs:
    target: ResolvedTarget
    artifacts: List[ArtifactRecord] = field(default_factory=list)
    build_dir: Optional[Path] = None
    manifest_path: Optional[Path] = None


@dataclass
class PackagePlan:
    target: ResolvedTarget
    build_outputs: BuildOutputs
    packaged_dir: Path
    manifest_dir: Path


@dataclass
class PackageOutputs:
    target: ResolvedTarget
    artifacts: List[ArtifactRecord] = field(default_factory=list)
    manifest_paths: Dict[str, Path] = field(default_factory=dict)


@dataclass
class RunPlan:
    target: ResolvedTarget
    run_manifest_path: Path
    log_dir: Path


@dataclass
class FlashPlan:
    target: ResolvedTarget
    flash_manifest_path: Path
    dry_run: bool = False


@dataclass
class DebugPlan:
    target: ResolvedTarget
    debug_manifest_path: Path
