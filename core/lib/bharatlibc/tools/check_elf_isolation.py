#!/usr/bin/env python3
import os
import sys
import re

PROHIBITED_PATTERNS = [
    r'core/kernel',
    r'core/hal',
    r'core/arch',
    r'core/services',
    r'core/drivers',
    r'\.\./\.\./\.\./kernel',
    r'\.\./\.\./\.\./hal',
    r'\.\./\.\./\.\./arch',
    r'\.\./\.\./\.\./services',
    r'\.\./\.\./\.\./drivers',
]

def check_isolation():
    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    src_dir = os.path.join(base_dir, "src")
    inc_dir = os.path.join(base_dir, "include")

    files_to_check = []
    for root, _, files in os.walk(src_dir):
        for f in files:
            if f.endswith((".c", ".h", ".S")):
                files_to_check.append(os.path.join(root, f))
    for root, _, files in os.walk(inc_dir):
        for f in files:
            if f.endswith(".h"):
                files_to_check.append(os.path.join(root, f))

    success = True
    for filepath in files_to_check:
        with open(filepath, "r", errors="ignore") as f:
            lines = f.readlines()

        for idx, line in enumerate(lines, 1):
            if line.strip().startswith("#include"):
                # Check prohibited patterns
                for pat in PROHIBITED_PATTERNS:
                    if re.search(pat, line):
                        print(f"[-] Isolation violation in {os.path.relpath(filepath, base_dir)}:{idx}")
                        print(f"    Line: {line.strip()}")
                        print(f"    Reason: Matches prohibited pattern '{pat}'")
                        success = False

    if not success:
        sys.exit(1)
    print("All files passed isolation boundary checks! No kernel-private headers included.")

if __name__ == "__main__":
    check_isolation()
