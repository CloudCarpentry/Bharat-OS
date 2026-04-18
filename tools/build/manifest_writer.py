import json
from pathlib import Path

from tools.build.models import BuildOutputs, PackageOutputs, ResolvedTarget


def write_build_manifest(outputs: BuildOutputs) -> Path:
    manifest_path = outputs.manifest_path

    data = {
        "target_name": outputs.target.name,
        "build_dir": str(outputs.build_dir),
        "artifacts": [
            {
                "kind": a.kind,
                "path": str(a.path),
                "producer": a.producer,
                "description": a.description,
                "metadata": a.metadata
            }
            for a in outputs.artifacts
        ]
    }

    with open(manifest_path, "w") as f:
        json.dump(data, f, indent=2)

    print(f"[Build] Emitted build manifest to {manifest_path}")
    return manifest_path


def write_run_manifest(target: ResolvedTarget, package_outputs: PackageOutputs, path: Path) -> Path:
    artifacts = {}
    for a in package_outputs.artifacts:
        # Very simple map for now. A real implementation might filter by kind.
        if a.kind == "run_boot_artifact":
            artifacts["boot_artifact"] = str(a.path)
        elif a.kind == "kernel_elf":
            artifacts["canonical_elf"] = str(a.path)

    if not "boot_artifact" in artifacts:
        # Fallback to kernel elf if no transform produced a specific boot artifact
        artifacts["boot_artifact"] = artifacts.get("canonical_elf", "")

    run_cfg = target.run
    data = {
        "target_name": target.name,
        "arch": target.arch,
        "board": target.board,
        "runner_type": run_cfg.backend,
        "run_config": {
            "machine": run_cfg.machine,
            "cpu": run_cfg.cpu,
            "memory": run_cfg.memory,
            "smp": 1, # Defaulting for now
            "serial": run_cfg.serial,
            "display": run_cfg.display,
            "nographic": run_cfg.nographic,
            "extra_args": run_cfg.extra_args
        },
        "artifacts": artifacts,
        "boot_contract": {
            "protocol": target.boot.protocol,
            "dtb": {
                "mode": target.boot.dtb.mode
            }
        }
    }

    with open(path, "w") as f:
        json.dump(data, f, indent=2)

    print(f"[Package] Emitted run manifest to {path}")
    return path


def write_flash_manifest(target: ResolvedTarget, package_outputs: PackageOutputs, path: Path) -> Path:
    artifacts = {}
    for a in package_outputs.artifacts:
        if a.kind == "flash_artifact":
            artifacts["flash_artifact"] = str(a.path)

    if not "flash_artifact" in artifacts:
        artifacts["flash_artifact"] = ""

    flash_cfg = target.flash
    data = {
        "target_name": target.name,
        "flash_config": {
            "backend": flash_cfg.backend,
            "probe": flash_cfg.probe,
            "verify": flash_cfg.verify,
            "reset_after_flash": flash_cfg.reset_after_flash,
        },
        "artifacts": artifacts
    }

    with open(path, "w") as f:
        json.dump(data, f, indent=2)

    print(f"[Package] Emitted flash manifest to {path}")
    return path


def write_debug_manifest(target: ResolvedTarget, package_outputs: PackageOutputs, path: Path) -> Path:
    artifacts = {}
    for a in package_outputs.artifacts:
        if a.kind == "kernel_elf":
            artifacts["canonical_elf"] = str(a.path)

    artifacts["symbols"] = artifacts.get("canonical_elf", "")

    debug_cfg = target.debug
    data = {
        "target_name": target.name,
        "debug_config": {
            "backend": debug_cfg.backend,
            "gdb_port": debug_cfg.gdb_port,
            "stop_on_entry": debug_cfg.stop_on_entry,
        },
        "artifacts": artifacts
    }

    with open(path, "w") as f:
        json.dump(data, f, indent=2)

    print(f"[Package] Emitted debug manifest to {path}")
    return path
