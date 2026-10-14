#!/usr/bin/env python3
"""
Bharat-OS Automated Agent Verifier
===================================
Unified verification gate runner for autonomous coding agents (Jules, Antigravity, Copilot, Claude).
Supports:
- Static linters & contract checks
- Host unit & integration test suites
- 5-Architecture canonical headless matrix
- Multi-architecture GUI smoke tests
- Device profile qualification (desktop, mobile, automotive, rtos, drone/edge)
"""

import argparse
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Dict, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

PROFILE_TARGETS = {
    "desktop": [
        ("x86_64 Desktop Headless", "delivery/targets/qemu/x86_64_desktop_headless.yaml", "qemu-system-x86_64"),
        ("ARM64 Desktop Headless", "delivery/targets/qemu/arm64_desktop_headless.yaml", "qemu-system-aarch64"),
        ("RISC-V 64 Desktop Headless", "delivery/targets/qemu/riscv64_desktop_headless.yaml", "qemu-system-riscv64"),
    ],
    "mobile": [
        ("ARM64 Mobile Headless", "delivery/targets/qemu/arm64_mobile_headless.yaml", "qemu-system-aarch64"),
        ("RISC-V 64 Mobile Headless", "delivery/targets/qemu/riscv64_mobile_headless.yaml", "qemu-system-riscv64"),
        ("ARM64 Mobile GUI", "delivery/targets/qemu/arm64_mobile_gui.yaml", "qemu-system-aarch64"),
        ("RISC-V 64 Mobile GUI", "delivery/targets/qemu/riscv64_mobile_gui.yaml", "qemu-system-riscv64"),
    ],
    "automotive": [
        ("ARM64 Automotive ECU Headless", "delivery/targets/qemu/arm64_automotive_headless.yaml", "qemu-system-aarch64"),
    ],
    "rtos": [
        ("ARM32 RTOS MPU Headless", "delivery/targets/qemu/arm32_rtos_mpu_headless.yaml", "qemu-system-arm"),
        ("RISC-V 32 RTOS MPU Headless", "delivery/targets/qemu/riscv32_rtos_mpu_headless.yaml", "qemu-system-riscv32"),
    ],
    "drone": [
        ("ARM32 Edge/Drone Headless", "delivery/targets/qemu/arm32_edge_headless.yaml", "qemu-system-arm"),
        ("RISC-V 32 Edge/Drone Headless", "delivery/targets/qemu/riscv32_edge_headless.yaml", "qemu-system-riscv32"),
        ("ARM32 Edge/Drone GUI", "delivery/targets/qemu/arm32_edge_gui.yaml", "qemu-system-arm"),
        ("RISC-V 32 Edge/Drone GUI", "delivery/targets/qemu/riscv32_edge_gui.yaml", "qemu-system-riscv32"),
    ],
    "edge": [
        ("ARM32 Edge Headless", "delivery/targets/qemu/arm32_edge_headless.yaml", "qemu-system-arm"),
        ("RISC-V 32 Edge Headless", "delivery/targets/qemu/riscv32_edge_headless.yaml", "qemu-system-riscv32"),
        ("ARM32 Edge GUI", "delivery/targets/qemu/arm32_edge_gui.yaml", "qemu-system-arm"),
        ("RISC-V 32 Edge GUI", "delivery/targets/qemu/riscv32_edge_gui.yaml", "qemu-system-riscv32"),
    ],
}

GUI_TARGETS = [
    ("x86_64 Desktop GUI", "delivery/targets/qemu/x86_64_desktop_gui.yaml", "qemu-system-x86_64"),
    ("ARM64 Desktop GUI", "delivery/targets/qemu/arm64_desktop_gui.yaml", "qemu-system-aarch64"),
    ("RISC-V 64 Desktop GUI", "delivery/targets/qemu/riscv64_desktop_gui.yaml", "qemu-system-riscv64"),
    ("ARM32 Edge GUI", "delivery/targets/qemu/arm32_edge_gui.yaml", "qemu-system-arm"),
    ("RISC-V 32 Edge GUI", "delivery/targets/qemu/riscv32_edge_gui.yaml", "qemu-system-riscv32"),
]


def run_command(cmd: List[str], description: str, timeout_sec: int = 300) -> Tuple[str, str, int]:
    """Runs command and returns (status, output, returncode)."""
    print(f"\n[Verify] Running {description}: {' '.join(cmd)}")
    try:
        proc = subprocess.run(
            cmd,
            cwd=REPO_ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=timeout_sec
        )
        status = "PASS" if proc.returncode == 0 else "FAIL"
        return status, proc.stdout, proc.returncode
    except subprocess.TimeoutExpired:
        return "TIMEOUT", f"Command timed out after {timeout_sec} seconds", -1
    except FileNotFoundError as e:
        return "BLOCKED", f"Tool or script not found: {e}", -1
    except Exception as e:
        return "ERROR", f"Unexpected execution error: {e}", -1


def check_abi_modified(changed_files: List[str]) -> bool:
    for f in changed_files:
        fn = f.replace("\\", "/").lower()
        if "syscall" in fn or "abi" in fn or "interface/contracts/abi" in fn:
            return True
    return False


def check_memory_modified(changed_files: List[str]) -> bool:
    for f in changed_files:
        fn = f.replace("\\", "/").lower()
        if "mm/" in fn or "vmm" in fn or "pmm" in fn or "tlb" in fn or "aspace" in fn:
            return True
    return False


def check_display_modified(changed_files: List[str]) -> bool:
    for f in changed_files:
        fn = f.replace("\\", "/").lower()
        if "display" in fn or "ui" in fn or "gfx" in fn or "input" in fn:
            return True
    return False


def execute_target_smoke(name: str, yaml_rel: str, emulator: str, build_only: bool) -> Tuple[str, str, int]:
    yaml_path = REPO_ROOT / yaml_rel
    if not yaml_path.exists():
        return "BLOCKED", f"Target YAML not found: {yaml_rel}", -1

    if not build_only and not shutil.which(emulator):
        return "BLOCKED", f"Emulator {emulator} not installed on host PATH", -1

    action = "build" if build_only else "all"
    cmd = [sys.executable, "tools/build.py", action, "--target-yaml", str(yaml_path)]
    if not build_only:
        cmd.append("--smoke")

    return run_command(cmd, f"{name} ({action})")


def main():
    parser = argparse.ArgumentParser(description="Bharat-OS Automated Agent Verifier")
    parser.add_argument("--changed-files", nargs="*", default=[], help="Files modified in task")
    parser.add_argument("--linters-only", action="store_true", help="Run only static linters and contract checks")
    parser.add_argument("--matrix-only", action="store_true", help="Run only the 5-target canonical QEMU matrix")
    parser.add_argument("--gui-smoke", action="store_true", help="Run GUI smoke tests across desktop and mobile profiles")
    parser.add_argument("--profile-smoke", choices=["desktop", "mobile", "automotive", "rtos", "edge", "drone", "all"],
                        help="Run qualification smoke tests for specific device profile")
    parser.add_argument("--host-tests", action="store_true", help="Run fast host unit test suite via ctest")
    parser.add_argument("--build-only", action="store_true", help="Only build targets, do not execute QEMU emulators")
    parser.add_argument("--all", action="store_true", help="Run full qualification (linters, ABI, headless matrix, GUI smoke)")

    args = parser.parse_args()
    if not (args.linters_only or args.matrix_only or args.gui_smoke or args.profile_smoke or args.host_tests or args.all or args.changed_files):
        args.linters_only = True

    results: List[Dict[str, str]] = []
    has_failure = False

    # 1. Static Linters
    if args.linters_only or args.all or args.changed_files:
        status, out, code = run_command([sys.executable, "tools/agents/check_agent_config.py"], "Agent Config & Skills Linter")
        results.append({"Gate": "Agent Config", "Status": status, "Detail": "check_agent_config.py"})
        if code != 0:
            has_failure = True
            print(out)

        status, out, code = run_command([sys.executable, "tools/lint/check_layer_references.py"], "Layer References Linter")
        results.append({"Gate": "Layer References", "Status": status, "Detail": "check_layer_references.py"})
        if code != 0:
            has_failure = True
            print(out)

        status, out, code = run_command([sys.executable, "tools/lint/check_display_tiers.py"], "ADR-035 Display Tier Validator")
        results.append({"Gate": "Display Tiers", "Status": status, "Detail": "check_display_tiers.py"})
        if code != 0:
            has_failure = True
            print(out)

        if args.all or check_abi_modified(args.changed_files):
            status, out, code = run_command([sys.executable, "tools/abi/syscall_abi.py", "--check"], "Syscall ABI Lock Check")
            results.append({"Gate": "Syscall ABI", "Status": status, "Detail": "syscall_abi.py --check"})
            if code != 0:
                has_failure = True
                print(out)
        else:
            results.append({"Gate": "Syscall ABI", "Status": "SKIPPED", "Detail": "No ABI changes in task"})

        if args.all or check_memory_modified(args.changed_files):
            status, out, code = run_command([sys.executable, "tools/lint/check_no_recursive_memops.py"], "No-Recursive Memops Check")
            results.append({"Gate": "No-Recursive Memops", "Status": status, "Detail": "check_no_recursive_memops.py"})
            if code != 0:
                has_failure = True
                print(out)

    # 2. Host Test Suites
    if args.host_tests or args.all:
        status, out, code = run_command(["ctest", "--preset", "host-test", "--output-on-failure"], "Host Test Suite (CTest)")
        results.append({"Gate": "Host Tests", "Status": status, "Detail": "ctest --preset host-test"})
        if code != 0:
            has_failure = True
            print(out)

    # 3. Canonical 5-Arch Headless Matrix
    if args.matrix_only or args.all:
        matrix_cmd = [sys.executable, "tools/run_qemu_matrix.py", "--headless", "--smoke", "--all-arch"]
        if args.build_only:
            matrix_cmd.append("--build-only")
        status, out, code = run_command(matrix_cmd, "5-Architecture Canonical Headless Matrix")
        results.append({"Gate": "5-Arch Headless Matrix", "Status": status, "Detail": "run_qemu_matrix.py --all-arch"})
        if code != 0:
            has_failure = True
            print(out)

    # 4. GUI Smoke Tests
    if args.gui_smoke or args.all:
        print("\n--- Running Multi-Architecture GUI Smoke Gate ---")
        for name, yaml_rel, emulator in GUI_TARGETS:
            status, out, code = execute_target_smoke(name, yaml_rel, emulator, args.build_only)
            results.append({"Gate": f"GUI: {name}", "Status": status, "Detail": yaml_rel})
            if status == "FAIL":
                has_failure = True
                print(out)

    # 5. Profile-Specific Smoke Gate (Desktop, Mobile, Automotive, RTOS, Drone/Edge)
    if args.profile_smoke:
        profiles_to_run = list(PROFILE_TARGETS.keys()) if args.profile_smoke == "all" else [args.profile_smoke]
        for prof in profiles_to_run:
            print(f"\n--- Running Device Profile Qualification Gate: {prof.upper()} ---")
            for name, yaml_rel, emulator in PROFILE_TARGETS[prof]:
                status, out, code = execute_target_smoke(name, yaml_rel, emulator, args.build_only)
                results.append({"Gate": f"Profile [{prof}]: {name}", "Status": status, "Detail": yaml_rel})
                if status == "FAIL":
                    has_failure = True
                    print(out)

    # Print Summary Report Table
    print("\n" + "=" * 75)
    print("Bharat-OS Automated Verification Report")
    print("=" * 75)
    print(f"{'Gate / Subsystem':<35} | {'Status':<10} | {'Detail':<25}")
    print("-" * 75)
    for res in results:
        print(f"{res['Gate']:<35} | {res['Status']:<10} | {res['Detail']:<25}")
    print("=" * 75)

    if has_failure:
        print("\n[VERDICT] Verification FAILED or BLOCKED.")
        sys.exit(1)
    else:
        print("\n[VERDICT] Verification PASSED cleanly.")


if __name__ == "__main__":
    main()
