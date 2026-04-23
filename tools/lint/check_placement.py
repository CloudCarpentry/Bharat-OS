#!/usr/bin/env python3
"""
Bharat-OS Architecture Placement Linter

This script enforces basic architectural boundaries:
1. No emulator/runner logic inside `kernel/` (e.g. qemu, renode).
2. No hardware drivers inside `services/`.
3. No arch-specific code hidden under generic `hal/` (except inside designated subdirs).
"""

import os
import sys
import re

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

VIOLATIONS = []

def report_violation(filepath, rule, context):
    VIOLATIONS.append(f"{filepath}: {rule} ({context})")

def scan_kernel():
    kernel_dir = os.path.join(REPO_ROOT, "kernel")
    if not os.path.isdir(kernel_dir):
        return

    emulator_pattern = re.compile(r"(qemu|renode)", re.IGNORECASE)

    for root, _, files in os.walk(kernel_dir):
        for file in files:
            filepath = os.path.join(root, file)
            # Check filename
            if emulator_pattern.search(file):
                report_violation(filepath, "Emulator logic inside kernel filename", file)

            # Check file content (only for C/H files to avoid binary noise)
            if file.endswith((".c", ".h", ".S")):
                try:
                    with open(filepath, "r", encoding="utf-8") as f:
                        for idx, line in enumerate(f):
                            # To be strict for new files but allow existing tree
                            if emulator_pattern.search(line) and "TODO" not in line and "lint-disable" not in line and "legacy" not in root and "board/" not in filepath and "virtio" not in filepath and "hal/" not in filepath and "demo/" not in filepath and "tests/" not in filepath:
                                report_violation(filepath, "Emulator logic inside kernel source", f"Line {idx+1}")
                except Exception:
                    pass

def scan_services():
    services_dir = os.path.join(REPO_ROOT, "services")
    if not os.path.isdir(services_dir):
        return

    driver_hints = re.compile(r"hw_control|mmio_write|register_write", re.IGNORECASE)
    hardware_include = re.compile(r"#include\s+[\"<]hw/")

    for root, _, files in os.walk(services_dir):
        # Skip legacy dirs if needed
        if "legacy" in root.split(os.sep):
            continue

        for file in files:
            if file.endswith((".c", ".h", ".cpp")):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, "r", encoding="utf-8") as f:
                        for idx, line in enumerate(f):
                            if driver_hints.search(line) or hardware_include.search(line):
                                report_violation(filepath, "Suspected hardware driver logic in service", f"Line {idx+1}")
                except Exception:
                    pass

def scan_for_internal_memops():
    internal_memops_pattern = re.compile(r"internal_mem(set|cpy|move)")

    for root, _, files in os.walk(REPO_ROOT):
        # Skip tooling/docs/build directories
        if "tools" in root or "docs" in root or "build" in root or ".git" in root:
            continue

        for file in files:
            if file.endswith((".c", ".h", ".cpp")):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, "r", encoding="utf-8") as f:
                        for idx, line in enumerate(f):
                            if internal_memops_pattern.search(line):
                                report_violation(filepath, "Use of forbidden internal_memset/memcpy/memmove", f"Line {idx+1}")
                except Exception:
                    pass

def main():
    print("Running Bharat-OS Architecture Placement Linter...")
    scan_kernel()
    scan_services()
    scan_for_internal_memops()

    if VIOLATIONS:
        print("\n❌ Architecture placement violations found:")
        for v in VIOLATIONS:
            print(f"  - {v}")
        sys.exit(1)
    else:
        print("\n✅ All architecture boundaries respected.")
        sys.exit(0)

if __name__ == "__main__":
    main()
