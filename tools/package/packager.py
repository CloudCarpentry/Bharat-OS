import os
import json
from tools.package.artifact_registry import ArtifactRegistry

def package(target_spec, build_dir):
    """
    Executes the packaging layer for a target.
    Derives artifacts and generates manifests based on the declarative target spec.
    """
    print(f"\n[Package] Starting packaging for {target_spec.get('name')}")

    registry = ArtifactRegistry()

    canonical_elf = target_spec.get('kernel', {}).get('canonical_artifact')
    if canonical_elf:
        canonical_elf = os.path.join(build_dir, canonical_elf)
    else:
        # Fallback for legacy mode
        canonical_elf = os.path.join(build_dir, 'kernel', 'kernel.elf')

    packaged_dir = os.path.join(build_dir, 'packaged')
    os.makedirs(packaged_dir, exist_ok=True)

    boot_artifact = canonical_elf # Default to elf if no transform

    transforms = target_spec.get('package', {}).get('transforms', [])
    for transform in transforms:
        t_type = transform.get('type')
        t_input = transform.get('input')
        t_output = transform.get('output')

        # Replace template placeholders like {{build_dir}} if we want, or just assume relative paths to build dir
        input_path = os.path.join(build_dir, t_input) if not os.path.isabs(t_input) else t_input
        output_path = os.path.join(build_dir, t_output) if not os.path.isabs(t_output) else t_output

        try:
            registry.apply_transform(t_type, input_path, output_path)
            # If this is the main output (e.g., bin), it's likely the boot artifact
            if output_path.endswith('.bin'):
                boot_artifact = output_path
        except Exception as e:
            print(f"Error applying transform {t_type}: {e}")
            raise e

    # Generate Run Manifest
    run_spec = target_spec.get('run', {})
    if run_spec:
        # If run spec specifies a boot artifact, use it. Otherwise, guess based on transform.
        spec_boot_artifact = run_spec.get('boot_artifact')
        if spec_boot_artifact:
            boot_artifact = os.path.join(build_dir, spec_boot_artifact) if not os.path.isabs(spec_boot_artifact) else spec_boot_artifact

        run_manifest = {
            "target_name": target_spec.get('name'),
            "arch": target_spec.get('arch'),
            "board": target_spec.get('board'),
            "runner_type": run_spec.get('method', 'qemu'),
            "run_config": {
                "machine": run_spec.get('machine'),
                "cpu": run_spec.get('cpu'),
                "memory": run_spec.get('memory'),
                "smp": run_spec.get('smp'),
                "serial": run_spec.get('serial', ['stdio']),
                "display": run_spec.get('display', 'none'),
                "extra_args": run_spec.get('extra_args', [])
            },
            "artifacts": {
                "boot_artifact": boot_artifact,
                "canonical_elf": canonical_elf
            },
            "boot_contract": target_spec.get('boot', {})
        }

        manifests_dir = os.path.join(build_dir, 'manifests')
        os.makedirs(manifests_dir, exist_ok=True)
        run_manifest_path = os.path.join(manifests_dir, 'run-manifest.json')
        with open(run_manifest_path, 'w') as f:
            json.dump(run_manifest, f, indent=2)
        print(f"[Package] Generated run manifest: {run_manifest_path}")

    return {
        "boot_artifact": boot_artifact,
        "canonical_elf": canonical_elf,
        "run_manifest": run_manifest_path if 'run_spec' in locals() and run_spec else None
    }
