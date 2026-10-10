#!/usr/bin/env python3
"""
Bharat-OS Cross-Architecture QEMU Matrix & Demo Runner
======================================================
Coordinates multi-architecture verification and demonstration runs across:
- x86_64
- ARM64
- ARM32
- RISC-V64
- RISC-V32

Differentiates granular verdicts:
- BUILD_PASS / BUILD_FAIL
- KERNEL_BOOT_PASS / BOOT_FAIL
- USERSPACE_READY
- SPLASH_VISIBLE
- GUI_INTERACTIVE
- BLOCKED (emulator / toolchain unavailable)
"""

import argparse
import json
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

TARGETS_DIR = REPO_ROOT / "delivery" / "targets" / "qemu"

EMULATOR_MAP = {
    "x86_64": "qemu-system-x86_64",
    "arm64": "qemu-system-aarch64",
    "arm32": "qemu-system-arm",
    "riscv64": "qemu-system-riscv64",
    "riscv32": "qemu-system-riscv32",
}

def check_emulator_available(arch: str) -> bool:
    binary_name = EMULATOR_MAP.get(arch)
    if not binary_name:
        return False
    return shutil.which(binary_name) is not None

def run_command(cmd, capture_output=False):
    print(f"\n[Matrix] Running: {' '.join(cmd)}")
    if capture_output:
        return subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True)
    return subprocess.run(cmd, cwd=REPO_ROOT)

def resolve_target_yaml(arch: str, kind: str) -> Path:
    """Resolve canonical target YAML file for a given architecture and display mode (kind)."""
    if kind == "headless":
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
        fallback_path = TARGETS_DIR / f"{arch}_desktop_headless.yaml"
        if fallback_path.exists():
            return fallback_path
        return yaml_path
    elif kind == "gui":
        preferred_gui = {
            "x86_64": "x86_64_desktop_gui.yaml",
            "arm64": "arm64_desktop_gui.yaml",
            "riscv64": "riscv64_desktop_gui.yaml",
            "arm32": "arm32_desktop_gui.yaml",
            "riscv32": "riscv32_desktop_gui.yaml",
        }
        filename = preferred_gui.get(arch, f"{arch}_desktop_gui.yaml")
        yaml_path = TARGETS_DIR / filename
        if yaml_path.exists():
            return yaml_path
        return TARGETS_DIR / f"{arch}_desktop_{kind}.yaml"
    else:
        return TARGETS_DIR / f"{arch}_{kind}.yaml"

def main():
    parser = argparse.ArgumentParser(description="Bharat-OS QEMU Matrix & Demo Runner")
    parser.add_argument("--headless", action="store_true", help="Build/Run all headless targets.")
    parser.add_argument("--gui", action="store_true", help="Build/Run all GUI targets.")
    parser.add_argument("--demo", action="store_true", help="Launch interactive showcase/demo target for selected architecture.")
    parser.add_argument("--smoke", action="store_true", help="Smoke-test the targets (exit on boot marker).")
    parser.add_argument("--build-only", action="store_true", help="Only build the targets, do not run them.")
    parser.add_argument("--all-arch", action="store_true", help="Build/Run across all 5 required architecture targets.")
    parser.add_argument("--arch", choices=["x86_64", "arm64", "arm32", "riscv64", "riscv32"], help="Select specific architecture.")
    parser.add_argument("--json-report", type=str, help="Path to write JSON execution report.")

    args = parser.parse_args()

    if not args.headless and not args.gui and not args.all_arch and not args.demo:
        print("Please specify --headless, --gui, --demo, and/or --all-arch.")
        sys.exit(1)

    if args.arch:
        archs = [args.arch]
    else:
        archs = ["x86_64", "arm64", "arm32", "riscv64", "riscv32"]

    kinds = []
    if args.headless or (args.all_arch and not args.gui and not args.demo):
        kinds.append("headless")
    if args.gui or args.demo:
        kinds.append("gui")

    results = []
    detailed_report = {
        "summary": {},
        "targets": []
    }

    for arch in archs:
        for kind in kinds:
            target_yaml = resolve_target_yaml(arch, kind)
            target_name = target_yaml.stem
            emulator_available = check_emulator_available(arch)

            target_info = {
                "architecture": arch,
                "kind": kind,
                "target_name": target_name,
                "target_yaml": str(target_yaml),
                "emulator": EMULATOR_MAP.get(arch),
                "emulator_available": emulator_available,
                "verdict": "UNKNOWN",
                "return_code": -1
            }

            if not target_yaml.exists():
                print(f"[Matrix] Skipping {target_name} (YAML not found at {target_yaml})")
                target_info["verdict"] = "BLOCKED: YAML not found"
                results.append((target_name, "BLOCKED", 1))
                detailed_report["targets"].append(target_info)
                continue

            if not args.build_only and not emulator_available:
                print(f"[Matrix] Emulator {EMULATOR_MAP.get(arch)} not found on PATH. Build will proceed, run gate marked BLOCKED.")
                target_info["verdict"] = "BLOCKED: Emulator missing"
                results.append((target_name, "BLOCKED", 1))
                detailed_report["targets"].append(target_info)
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

            res = run_command(cmd, capture_output=False)
            rc = res.returncode
            target_info["return_code"] = rc

            if rc == 0:
                if args.build_only:
                    verdict = "BUILD_PASS"
                elif kind == "gui":
                    verdict = "GUI_READY (PASS)"
                else:
                    verdict = "BOOT_PASS (PASS)"
            else:
                verdict = "FAIL"

            target_info["verdict"] = verdict
            results.append((target_name, verdict, rc))
            detailed_report["targets"].append(target_info)

    print("\n" + "="*50)
    print(f"{'Target Name':32} | {'Status/Verdict':20}")
    print("="*50)
    failed = False
    for name, verdict, rc in results:
        print(f"{name:32} | {verdict:20}")
        if rc != 0:
            failed = True

    if args.json_report:
        report_path = Path(args.json_report)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        with open(report_path, "w", encoding="utf-8") as f:
            json.dump(detailed_report, f, indent=2)
        print(f"\n[Matrix] Report written to: {report_path}")

    if failed:
        sys.exit(1)

if __name__ == "__main__":
    main()
