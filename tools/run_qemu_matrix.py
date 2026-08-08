#!/usr/bin/env python3
import argparse
import subprocess
import sys
from pathlib import Path

# Add repo root to sys.path so we can import from tools.*
REPO_ROOT = Path(__file__).resolve().parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

TARGETS_DIR = REPO_ROOT / "delivery" / "targets" / "qemu"

def run_command(cmd):
    print(f"\n[Matrix] Running: {' '.join(cmd)}")
    return subprocess.run(cmd, cwd=REPO_ROOT).returncode

def resolve_target_yaml(arch: str, kind: str) -> Path:
    """Resolve the canonical target YAML file for a given architecture and display mode (kind)."""
    if kind == "headless":
        # Preferred headless targets per architecture contract (AGENTS.md)
        preferred_headless = {
            "arm32": "arm32_mmu_lite_headless.yaml",
            "riscv32": "riscv32_mmu_lite_headless.yaml",
            "x86_64": "x86_64_desktop_headless.yaml",
            "arm64": "arm64_desktop_headless.yaml",
            "riscv64": "riscv64_desktop_headless.yaml",
        }
        filename = preferred_headless.get(arch, f"{arch}_desktop_headless.yaml")
        yaml_path = TARGETS_DIR / filename
        if yaml_path.exists():
            return yaml_path
        # Fallback to desktop_headless if mmu_lite variant doesn't exist
        fallback_path = TARGETS_DIR / f"{arch}_desktop_headless.yaml"
        if fallback_path.exists():
            return fallback_path
        return yaml_path
    else:
        return TARGETS_DIR / f"{arch}_desktop_{kind}.yaml"

def main():
    parser = argparse.ArgumentParser(description="Bharat-OS QEMU Matrix Runner")
    parser.add_argument("--headless", action="store_true", help="Build/Run all headless targets.")
    parser.add_argument("--gui", action="store_true", help="Build/Run all GUI targets.")
    parser.add_argument("--smoke", action="store_true", help="Smoke-test the targets (exit on boot marker).")
    parser.add_argument("--build-only", action="store_true", help="Only build the targets, do not run them.")
    parser.add_argument("--all-arch", action="store_true", help="Build/Run across all 5 required architecture targets.")

    args = parser.parse_args()

    if not args.headless and not args.gui and not args.all_arch:
        print("Please specify --headless, --gui, and/or --all-arch.")
        sys.exit(1)

    archs = ["x86_64", "arm64", "arm32", "riscv64", "riscv32"]
    kinds = []
    if args.headless or (args.all_arch and not args.gui):
        kinds.append("headless")
    if args.gui:
        kinds.append("gui")

    results = []

    for arch in archs:
        for kind in kinds:
            target_yaml = resolve_target_yaml(arch, kind)
            target_name = target_yaml.stem

            if not target_yaml.exists():
                print(f"[Matrix] Skipping {target_name} (YAML not found at {target_yaml})")
                if args.all_arch:
                    results.append((f"{arch}_{kind}", 1))
                continue

            cmd = [sys.executable, "tools/build.py"]

            if args.build_only:
                cmd.extend(["build", "--target-yaml", str(target_yaml)])
            else:
                cmd.extend(["all", "--target-yaml", str(target_yaml)])
                if args.smoke:
                    cmd.append("--smoke")
                else:
                    cmd.append("--interactive")

            rc = run_command(cmd)
            results.append((target_name, rc))

    print("\n" + "="*40)
    print("Matrix Results:")
    print("="*40)
    failed = False
    for name, rc in results:
        status = "PASS" if rc == 0 else "FAIL"
        print(f"{name:30} : {status}")
        if rc != 0:
            failed = True

    if failed:
        sys.exit(1)

if __name__ == "__main__":
    main()

