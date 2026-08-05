from pathlib import Path
import json

from tools.build.models import ArtifactRecord, BuildOutputs, PackageOutputs, PackagePlan, ResolvedTarget
from tools.build.paths import get_manifest_dir, get_packaged_dir
from tools.package.artifact_registry import ArtifactRegistry
from tools.build.manifest_writer import write_run_manifest, write_flash_manifest, write_debug_manifest


FDT_MAGIC = 0xD00DFEED
FDT_HEADER_SIZE = 40


def _compact_qemu_dtb(dtb_path: Path) -> None:
    """Remove QEMU dumpdtb padding so it cannot overlap the ARM64 kernel."""
    data = dtb_path.read_bytes()
    if len(data) < FDT_HEADER_SIZE:
        raise RuntimeError(f"QEMU generated a truncated DTB: {dtb_path}")

    magic = int.from_bytes(data[0:4], byteorder="big")
    total_size = int.from_bytes(data[4:8], byteorder="big")
    if magic != FDT_MAGIC or total_size < FDT_HEADER_SIZE or total_size > len(data):
        raise RuntimeError(f"QEMU generated an invalid DTB: {dtb_path}")

    struct_offset = int.from_bytes(data[8:12], byteorder="big")
    strings_offset = int.from_bytes(data[12:16], byteorder="big")
    reserve_offset = int.from_bytes(data[16:20], byteorder="big")
    strings_size = int.from_bytes(data[32:36], byteorder="big")
    struct_size = int.from_bytes(data[36:40], byteorder="big")

    reserve_end = reserve_offset
    while reserve_end + 16 <= total_size:
        address = int.from_bytes(data[reserve_end:reserve_end + 8], "big")
        size = int.from_bytes(data[reserve_end + 8:reserve_end + 16], "big")
        reserve_end += 16
        if address == 0 and size == 0:
            break
    else:
        raise RuntimeError(f"QEMU generated an invalid DTB reserve map: {dtb_path}")

    compact_size = max(FDT_HEADER_SIZE, reserve_end,
                       struct_offset + struct_size,
                       strings_offset + strings_size)
    if compact_size > total_size:
        raise RuntimeError(f"QEMU generated an invalid DTB layout: {dtb_path}")

    compact = bytearray(data[:compact_size])
    compact[4:8] = compact_size.to_bytes(4, byteorder="big")
    dtb_path.write_bytes(compact)


def make_package_plan(target: ResolvedTarget, build_outputs: BuildOutputs, repo_root: Path) -> PackagePlan:
    packaged_dir = get_packaged_dir(target, repo_root)
    manifest_dir = get_manifest_dir(target, repo_root)

    return PackagePlan(
        target=target,
        build_outputs=build_outputs,
        packaged_dir=packaged_dir,
        manifest_dir=manifest_dir
    )


def _candidate_service_binary_paths(build_dir: Path, binary_name: str) -> list[Path]:
    return [
        build_dir / "core" / "services" / "core" / binary_name / binary_name,
        build_dir / "services" / "core" / binary_name / binary_name,
        build_dir / "core" / "services" / binary_name / binary_name,
        build_dir / "services" / binary_name / binary_name,
    ]


def _find_required_service_binary(build_dir: Path, binary_name: str) -> Path:
    for path in _candidate_service_binary_paths(build_dir, binary_name):
        if path.is_file():
            return path
        exe_path = path.with_suffix(".exe")
        if exe_path.is_file():
            return exe_path

    candidates = ", ".join(str(path) for path in _candidate_service_binary_paths(build_dir, binary_name))
    raise RuntimeError(
        f"Required compiled payload 'services/{binary_name}' was not produced; "
        f"refusing to package a synthetic boot module. Checked: {candidates}"
    )


def execute_package(plan: PackagePlan, repo_root: Path) -> PackageOutputs:
    print(f"\n[Package] Starting packaging for {plan.target.name}")

    plan.packaged_dir.mkdir(parents=True, exist_ok=True)
    plan.manifest_dir.mkdir(parents=True, exist_ok=True)

    registry = ArtifactRegistry()

    packaged_artifacts = list(plan.build_outputs.artifacts)

    # Process transforms
    boot_artifact_path = None
    for transform_cfg in plan.target.package.transforms:
        # Resolve inputs/outputs relative to build dir usually
        input_path = plan.build_outputs.build_dir / transform_cfg.input
        output_path = plan.build_outputs.build_dir / transform_cfg.output

        try:
            print(f"[Package] Applying transform {transform_cfg.type}: {input_path} -> {output_path}")
            registry.apply_transform(transform_cfg.type, input_path, output_path)

            # If the output matches our boot_artifact requirement, store it
            if plan.target.run and plan.target.run.boot_artifact:
                if transform_cfg.output == plan.target.run.boot_artifact:
                    boot_artifact_path = output_path

            if plan.target.flash and plan.target.flash.artifact:
                if transform_cfg.output == plan.target.flash.artifact:
                    packaged_artifacts.append(
                        ArtifactRecord(kind="flash_artifact", path=output_path, producer=f"transform_{transform_cfg.type}")
                    )

        except Exception as e:
            print(f"Error applying transform {transform_cfg.type}: {e}")
            raise e

    if boot_artifact_path:
        packaged_artifacts.append(
            ArtifactRecord(kind="run_boot_artifact", path=boot_artifact_path, producer="packager")
        )
    elif plan.target.run and plan.target.run.boot_artifact:
        # Fallback for when the artifact wasn't generated by a transform, e.g. it is the canonical elf
        boot_artifact_path = plan.build_outputs.build_dir / plan.target.run.boot_artifact
        packaged_artifacts.append(
            ArtifactRecord(kind="run_boot_artifact", path=boot_artifact_path, producer="packager_fallback")
        )

    # -------------------------------------------------------------
    # Packaging of init_module (services/init or services/rt-supervisor)
    # -------------------------------------------------------------
    import struct

    is_rt_mpu = (plan.target.execution_profile == "rt" and
                 plan.target.build.cmake_defs.get("BHARAT_PROFILE_MPU_ONLY") == "ON")

    binary_name = "rt-supervisor" if is_rt_mpu else "init"
    src_binary = _find_required_service_binary(plan.build_outputs.build_dir, binary_name)

    init_module_path = plan.packaged_dir / "init_module.bin"

    payload_bytes = src_binary.read_bytes()
    print(f"[Package] Found compiled payload for 'services/{binary_name}' at {src_binary} ({len(payload_bytes)} bytes)")

    # Create the versioned Bharat boot-module container header:
    magic = 0xB4A2D1A5
    abi_version = 0x0100
    header_size = 128
    module_kind = 2 if is_rt_mpu else 1
    payload_offset = 128
    payload_size = len(payload_bytes)
    target_arch = 0
    elf_class = 0
    flags = 0
    name = f"services/{binary_name}"
    name_len = len(name)
    digest_algo = 0
    digest = b"\x00" * 32
    name_bytes = name.encode("utf-8")[:32].ljust(32, b"\x00")
    padding = b"\x00" * 20

    header = struct.pack(
        "<IIIIIIIIIII32s32s20s",
        magic, abi_version, header_size, module_kind, payload_offset, payload_size,
        target_arch, elf_class, flags, name_len, digest_algo, digest, name_bytes, padding
    )

    init_module_path.write_bytes(header + payload_bytes)
    print(f"[Package] Packaged and wrote Bharat boot-module container to {init_module_path}")

    packaged_artifacts.append(
        ArtifactRecord(kind="init_module", path=init_module_path, producer="packager_boot_module")
    )

    manifest_paths = {}

    outputs = PackageOutputs(
        target=plan.target,
        artifacts=packaged_artifacts,
        manifest_paths=manifest_paths
    )

    if plan.target.boot.dtb.required and plan.target.boot.dtb.mode == "qemu_generated":
        dtb_path = plan.manifest_dir / "hw.dtb"
        qemu_bin_map = {
            "x86_64": "qemu-system-x86_64",
            "arm64": "qemu-system-aarch64",
            "riscv64": "qemu-system-riscv64",
            "arm32": "qemu-system-arm",
            "riscv32": "qemu-system-riscv32",
        }
        qemu_exe = qemu_bin_map.get(plan.target.arch, f"qemu-system-{plan.target.arch}")
        dtb_cmd = [qemu_exe]
        if plan.target.run and plan.target.run.machine:
            dtb_cmd.extend(["-machine", plan.target.run.machine + ",dumpdtb=" + str(dtb_path)])

        if plan.target.run and plan.target.run.cpu:
            dtb_cmd.extend(["-cpu", plan.target.run.cpu])

        if plan.target.run and plan.target.run.smp:
            dtb_cmd.extend(["-smp", str(plan.target.run.smp)])

        print(f"[Package] Generating QEMU DTB: {' '.join(dtb_cmd)}")
        import subprocess
        subprocess.run(dtb_cmd, capture_output=True, check=True)
        if dtb_path.exists():
            # QEMU pads dumpdtb output (commonly to 1 MiB).  On ARM virt the
            # blob is handed off at RAM base while the kernel starts only
            # 512 KiB later, so retaining that padding creates an overlapping
            # boot artifact.  Keep only the FDT header's declared total size.
            _compact_qemu_dtb(dtb_path)
            packaged_artifacts.append(
                ArtifactRecord(kind="dtb", path=dtb_path, producer="qemu_dumpdtb")
            )

    if plan.target.run:
        run_manifest_path = plan.manifest_dir / "run-manifest.json"
        manifest_paths["run"] = write_run_manifest(plan.target, outputs, run_manifest_path)

    if plan.target.flash:
        flash_manifest_path = plan.manifest_dir / "flash-manifest.json"
        manifest_paths["flash"] = write_flash_manifest(plan.target, outputs, flash_manifest_path)

    if plan.target.debug:
        debug_manifest_path = plan.manifest_dir / "debug-manifest.json"
        manifest_paths["debug"] = write_debug_manifest(plan.target, outputs, debug_manifest_path)

    footprint_report = _write_footprint_report(plan, packaged_artifacts)
    manifest_paths["footprint_report"] = footprint_report

    return outputs


def _write_footprint_report(plan: PackagePlan, artifacts: list[ArtifactRecord]) -> Path:
    report_path = plan.manifest_dir / "footprint-report.json"
    by_kind = {}
    for artifact in artifacts:
        size = artifact.path.stat().st_size if artifact.path.exists() else 0
        by_kind.setdefault(artifact.kind, 0)
        by_kind[artifact.kind] += size

    payload = {
        "target": plan.target.name,
        "arch": plan.target.arch,
        "footprint_profile": plan.target.footprint_profile,
        "binary_size_breakdown_bytes": by_kind,
        "total_artifact_size_bytes": sum(by_kind.values()),
    }
    with open(report_path, "w", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)
    return report_path
