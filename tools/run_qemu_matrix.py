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

def main():
    parser = argparse.ArgumentParser(description="Bharat-OS QEMU Matrix Runner")
    parser.add_argument("--headless", action="store_true", help="Build/Run all headless targets.")
    parser.add_argument("--gui", action="store_true", help="Build/Run all GUI targets.")
    parser.add_argument("--smoke", action="store_true", help="Smoke-test the targets (exit on boot marker).")
    parser.add_argument("--build-only", action="store_true", help="Only build the targets, do not run them.")

    args = parser.parse_args()

    if not args.headless and not args.gui:
        print("Please specify --headless and/or --gui.")
        sys.exit(1)

    archs = ["x86_64", "arm64", "arm32", "riscv64", "riscv32"]
    kinds = []
    if args.headless:
        kinds.append("headless")
    if args.gui:
        kinds.append("gui")

    results = []

    for arch in archs:
        for kind in kinds:
            target_name = f"{arch}_desktop_{kind}"
            target_yaml = TARGETS_DIR / f"{target_name}.yaml"

            if not target_yaml.exists():
                print(f"[Matrix] Skipping {target_name} (YAML not found at {target_yaml})")
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
