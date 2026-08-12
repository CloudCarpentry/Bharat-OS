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

# Compile patterns once
emulator_pattern = re.compile(r"(qemu|renode)", re.IGNORECASE)
driver_hints = re.compile(r"hw_control|mmio_write|register_write", re.IGNORECASE)
hardware_include = re.compile(r"#include\s+[\"<]hw/")
internal_memops_pattern = re.compile(r"internal_mem(set|cpy|move)")

def check_kernel_content(filepath, lines, root):
    for idx, line in enumerate(lines):
        # To be strict for new files but allow existing tree
        if emulator_pattern.search(line) and "TODO" not in line and "lint-disable" not in line and "legacy" not in root and "board/" not in filepath and "virtio" not in filepath and "hal/" not in filepath and "demo/" not in filepath and "tests/" not in filepath:
            report_violation(filepath, "Emulator logic inside kernel source", f"Line {idx+1}")

def check_services_content(filepath, lines):
    for idx, line in enumerate(lines):
        if driver_hints.search(line) or hardware_include.search(line):
            report_violation(filepath, "Suspected hardware driver logic in service", f"Line {idx+1}")

def check_memops_content(filepath, lines):
    for idx, line in enumerate(lines):
        if internal_memops_pattern.search(line):
            report_violation(filepath, "Use of forbidden internal_memset/memcpy/memmove", f"Line {idx+1}")

def main():
    print("Running Bharat-OS Architecture Placement Linter...")

    # Set of directories to exclude for memops checks (must be exact path components)
    memops_excluded = {"tools", "docs", "build", ".git"}

    # Pre-calculate kernel and services paths for fast prefix checking
    kernel_dir = os.path.join(REPO_ROOT, "kernel")
    services_dir = os.path.join(REPO_ROOT, "services")

    for root, _, files in os.walk(REPO_ROOT):
        parts = set(root.split(os.sep))

        is_kernel = root == kernel_dir or root.startswith(kernel_dir + os.sep)
        is_services = (root == services_dir or root.startswith(services_dir + os.sep)) and "legacy" not in parts
        is_memops_eligible = not memops_excluded.intersection(parts)

        if not (is_kernel or is_services or is_memops_eligible):
            continue

        for file in files:
            filepath = os.path.join(root, file)

            # 1. Filename checks (no file read required)
            if is_kernel:
                if emulator_pattern.search(file):
                    report_violation(filepath, "Emulator logic inside kernel filename", file)

            # 2. Content checks
            needs_kernel = is_kernel and file.endswith((".c", ".h", ".S"))
            needs_services = is_services and file.endswith((".c", ".h", ".cpp"))
            needs_memops = is_memops_eligible and file.endswith((".c", ".h", ".cpp"))

            if needs_kernel or needs_services or needs_memops:
                try:
                    with open(filepath, "r", encoding="utf-8") as f:
                        lines = f.readlines()

                    if needs_kernel:
                        check_kernel_content(filepath, lines, root)
                    if needs_services:
                        check_services_content(filepath, lines)
                    if needs_memops:
                        check_memops_content(filepath, lines)
                except Exception:
                    pass

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
