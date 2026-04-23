import sys
import os
from pathlib import Path

# Add repo root to sys.path so we can import from tools.*
REPO_ROOT = Path(__file__).resolve().parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.build.cli import parse_args
from tools.build.target_resolver import resolve_target_input, resolve_yaml_target
from tools.build.legacy_adapter import resolve_legacy_target
from tools.build.validators import validate_resolved_target

from tools.build.build_executor import make_build_plan, execute_build
from tools.package.packager import make_package_plan, execute_package
from tools.run.runner_qemu import run_qemu

def load_existing_build_outputs(target, repo_root):
    from tools.build.models import BuildOutputs, ArtifactRecord
    from tools.build.paths import get_output_root
    build_dir = get_output_root(target, repo_root)
    # Mocking for simplicity if we didn't just build it.
    return BuildOutputs(
        target=target,
        build_dir=build_dir,
        artifacts=[ArtifactRecord(kind="kernel_elf", path=build_dir / target.kernel.canonical_elf, producer="manual")]
    )

def load_existing_package_outputs(target, repo_root):
    from tools.build.models import PackageOutputs
    from tools.build.paths import get_manifest_dir
    manifest_dir = get_manifest_dir(target, repo_root)
    return PackageOutputs(target=target, manifest_paths={"run": manifest_dir / "run-manifest.json"})

def main() -> int:
    args = parse_args()

    target_input = resolve_target_input(args.target, getattr(args, "target_yaml", None))

    repo_root = REPO_ROOT

    if target_input.source_kind == "yaml":
        target = resolve_yaml_target(Path(target_input.source_ref))
    else:
        target = resolve_legacy_target(target_input.source_ref, repo_root)

    validate_resolved_target(target, repo_root)

    build_outputs = None
    if args.command in ("configure", "build", "all"):
        build_plan = make_build_plan(target, repo_root)
        build_outputs = execute_build(build_plan, repo_root)
    else:
        build_outputs = load_existing_build_outputs(target, repo_root)

    package_outputs = None
    if args.command in ("package", "all", "run", "flash", "debug"):
        package_plan = make_package_plan(target, build_outputs, repo_root)
        package_outputs = execute_package(package_plan, repo_root)
    else:
        package_outputs = load_existing_package_outputs(target, repo_root)

    if args.command in ("run", "all"):
        if not target.run:
            print("Error: Target does not support 'run' action.")
            return 1
        return run_qemu(package_outputs.manifest_paths["run"])

    if args.command == "flash":
        if not target.flash:
            print("Error: Target does not support 'flash' action.")
            return 1
        from tools.flash.flasher import execute_flash
        return execute_flash(package_outputs.manifest_paths["flash"], dry_run=getattr(args, "dry_run", False))

    if args.command == "debug":
        if not target.debug:
            print("Error: Target does not support 'debug' action.")
            return 1
        print("Debug workflow not fully implemented yet.")
        return 0

    return 0

if __name__ == "__main__":
    sys.exit(main())
