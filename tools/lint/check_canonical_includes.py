#!/usr/bin/env python3
import os
import sys
import re

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

def main():
    print("Running Canonical Includes Linter...")

    violation_pattern = re.compile(r'#include\s+"(\.\./)+((kernel/include/hal|include/hal|interface/include)/[^"]+)"')
    violations = []

    for root, _, files in os.walk(os.path.join(REPO_ROOT, "core", "arch")):
        for file in files:
            if not file.endswith((".c", ".h", ".S")):
                continue

            filepath = os.path.join(root, file)
            try:
                with open(filepath, "r", encoding="utf-8") as f:
                    for idx, line in enumerate(f):
                        match = violation_pattern.search(line)
                        if match:
                            violations.append(f"{filepath}:{idx+1}: {line.strip()}")
            except Exception:
                pass

    if violations:
        print("\n[ERROR] Found uncanonical includes:")
        for v in violations:
            print(f"  - {v}")
        sys.exit(1)

    print("\n[OK] All architecture includes are canonical.")
    sys.exit(0)

if __name__ == "__main__":
    main()
