import subprocess
import sys
from pathlib import Path

from tools.build.models import ArtifactRecord, BuildOutputs, BuildPlan, ResolvedTarget
from tools.build.paths import get_manifest_dir, get_output_root
from tools.build.manifest_writer import write_build_manifest


def run_command(cmd: list[str], cwd: Path | None = None) -> None:
    print(f"Running: {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd)
    if result.returncode != 0:
        print(f"Command failed with exit code {result.returncode}")
        sys.exit(result.returncode)


def make_build_plan(target: ResolvedTarget, repo_root: Path) -> BuildPlan:
    build_dir = get_output_root(target, repo_root)
    manifest_dir = get_manifest_dir(target, repo_root)
    manifest_path = manifest_dir / "build-manifest.json"

    preset = target.build.cmake_preset
    defs = target.build.cmake_defs

    config_cmd = ["cmake", f"--preset={preset}"]
    for k, v in defs.items():
        config_cmd.append(f"-D{k}={v}")

    build_cmd = ["cmake", "--build", f"--preset={preset}"]

    return BuildPlan(
        target=target,
        build_dir=build_dir,
        manifest_path=manifest_path,
        command_summary=[config_cmd, build_cmd]
    )


def execute_build(plan: BuildPlan, repo_root: Path) -> BuildOutputs:
    print(f"\n[Build] Starting build for {plan.target.name}")

    for cmd in plan.command_summary:
        run_command(cmd, cwd=repo_root)

    # Collect canonical artifacts
    artifacts = []

    # Kernel ELF
    kernel_elf_rel = plan.target.kernel.canonical_elf
    kernel_elf_path = plan.build_dir / kernel_elf_rel

    # We might not error out if it doesn't exist yet (e.g. if we skip build step but still try to make outputs),
    # but in a real execute_build it should exist.

    artifacts.append(
        ArtifactRecord(
            kind="kernel_elf",
            path=kernel_elf_path,
            producer="build_executor"
        )
    )

    if plan.target.kernel.canonical_map:
        artifacts.append(
            ArtifactRecord(
                kind="kernel_map",
                path=plan.build_dir / plan.target.kernel.canonical_map,
                producer="build_executor"
            )
        )

    outputs = BuildOutputs(
        target=plan.target,
        artifacts=artifacts,
        build_dir=plan.build_dir,
        manifest_path=plan.manifest_path
    )

    # Emit build manifest
    plan.manifest_path.parent.mkdir(parents=True, exist_ok=True)
    write_build_manifest(outputs)

    return outputs
