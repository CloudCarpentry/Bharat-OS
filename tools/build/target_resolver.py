import sys
import yaml
import jsonschema
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
    TargetInput,
)

SCHEMA_PATH = Path(__file__).parent.parent / "schemas" / "target.schema.yaml"

def load_yaml_target(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"Target YAML not found: {path}")
    with open(path, "r") as f:
        return yaml.safe_load(f)

def validate_yaml_target(raw: dict) -> None:
    if not SCHEMA_PATH.exists():
        print(f"[Warning] Target schema not found at {SCHEMA_PATH}, skipping schema validation.")
        return

    with open(SCHEMA_PATH, "r") as f:
        schema = yaml.safe_load(f)

    try:
        jsonschema.validate(instance=raw, schema=schema)
    except jsonschema.exceptions.ValidationError as e:
        print(f"Schema Validation Error: {e.message}")
        sys.exit(1)

def resolve_yaml_target(path: Path) -> ResolvedTarget:
    raw = load_yaml_target(path)
    validate_yaml_target(raw)

    # Map raw -> ResolvedTarget
    build_raw = raw.get("build", {})
    build_cfg = BuildConfig(
        cmake_preset=build_raw.get("cmake_preset", "unknown"),
        cmake_defs=build_raw.get("cmake_defs", {})
    )

    kernel_raw = raw.get("kernel", {})
    kernel_artifacts = kernel_raw.get("canonical_artifacts", {})
    kernel_cfg = KernelConfig(
        canonical_elf=kernel_artifacts.get("elf", "kernel.elf"),
        canonical_map=kernel_artifacts.get("map"),
        canonical_symbols=kernel_artifacts.get("symbols"),
    )

    boot_raw = raw.get("boot", {})
    dtb_raw = boot_raw.get("dtb", {})
    boot_cfg = BootConfig(
        protocol=boot_raw.get("protocol", "unknown"),
        artifact_format=boot_raw.get("artifact_format", "unknown"),
        dtb=DtbConfig(
            mode=dtb_raw.get("mode", "unknown"),
            required=dtb_raw.get("required", False),
            path=dtb_raw.get("path"),
            handoff_register=dtb_raw.get("handoff_register")
        )
    )

    package_raw = raw.get("package", {})
    transforms = []
    for t in package_raw.get("transforms", []):
        transforms.append(PackageTransformConfig(
            type=t.get("type"),
            input=t.get("input"),
            output=t.get("output"),
            options=t.get("options", {})
        ))
    package_cfg = PackageConfig(transforms=transforms)

    run_raw = raw.get("run")
    run_cfg = None
    if run_raw:
        run_cfg = RunConfig(
            backend=run_raw.get("backend"),
            machine=run_raw.get("machine"),
            cpu=run_raw.get("cpu"),
            memory=run_raw.get("memory"),
            boot_artifact=run_raw.get("boot_artifact"),
            serial=run_raw.get("serial", []),
            display=run_raw.get("display"),
            nographic=run_raw.get("nographic", False),
            extra_args=run_raw.get("extra_args", [])
        )

    flash_raw = raw.get("flash")
    flash_cfg = None
    if flash_raw:
        flash_cfg = FlashConfig(
            backend=flash_raw.get("backend"),
            artifact=flash_raw.get("artifact"),
            probe=flash_raw.get("probe"),
            transport=flash_raw.get("transport"),
            verify=flash_raw.get("verify", False),
            reset_after_flash=flash_raw.get("reset_after_flash", False),
            extra_args=flash_raw.get("extra_args", [])
        )

    debug_raw = raw.get("debug")
    debug_cfg = None
    if debug_raw:
        debug_cfg = DebugConfig(
            backend=debug_raw.get("backend"),
            symbols=debug_raw.get("symbols"),
            gdb_port=debug_raw.get("gdb_port"),
            stop_on_entry=debug_raw.get("stop_on_entry", False),
            extra_args=debug_raw.get("extra_args", [])
        )

    return ResolvedTarget(
        name=raw.get("name", "unknown"),
        kind=raw.get("kind", "unknown"),
        arch=raw.get("arch", "unknown"),
        board=raw.get("board", "unknown"),
        device_profile=raw.get("device_profile", "unknown"),
        personality_profile=raw.get("personality_profile", "unknown"),
        execution_profile=raw.get("execution_profile"),
        build=build_cfg,
        kernel=kernel_cfg,
        boot=boot_cfg,
        package=package_cfg,
        run=run_cfg,
        flash=flash_cfg,
        debug=debug_cfg,
        source_metadata={"source_kind": "yaml", "source_path": str(path)}
    )

def resolve_target_input(target: str | None, target_yaml: str | None) -> TargetInput:
    if target_yaml:
        return TargetInput(source_kind="yaml", source_ref=target_yaml)
    elif target:
        return TargetInput(source_kind="legacy", source_ref=target)
    else:
        raise ValueError("Either --target or --target-yaml must be provided.")
