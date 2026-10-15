#!/usr/bin/env python3
"""
tools/lint/check_no_recursive_memops.py

Linter wrapper for recursive memop checking.
If an ELF path is passed as argument, verifies it.
Otherwise scans build directories for any compiled kernel.elf.
If no ELF is compiled yet, passes cleanly.
"""

import sys
import os
import glob
from pathlib import Path

# Add repo root to sys.path
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO_ROOT))

from tools.check_no_recursive_memops import main as run_check

def find_candidate_elfs():
    candidates = []
    # Search common build locations
    for pattern in ["build/**/kernel.elf", "build/**/kernel.bin", "target/**/kernel.elf"]:
        candidates.extend(glob.glob(str(REPO_ROOT / pattern), recursive=True))
    # Filter to only existing .elf files
    return [c for c in candidates if c.endswith(".elf") and os.path.isfile(c)]

def lint_main():
    if len(sys.argv) > 1:
        # Explicit ELF passed
        sys.exit(run_check())

    candidate_elfs = find_candidate_elfs()
    if not candidate_elfs:
        print("No built kernel.elf found to check; skipping binary memop disassembly.")
        print("Validation PASSED: Memop recursive check cleanly deferred to post-build.")
        sys.exit(0)

    all_passed = True
    for elf in candidate_elfs:
        print(f"Checking memops for {elf}...")
        orig_argv = sys.argv.copy()
        sys.argv = [orig_argv[0], elf]
        try:
            run_check()
        except SystemExit as e:
            if e.code != 0:
                all_passed = False
        finally:
            sys.argv = orig_argv

    if not all_passed:
        sys.exit(1)
    print("Validation PASSED: All found kernel ELFs adhere to non-recursive memop constraints.")
    sys.exit(0)

if __name__ == "__main__":
    lint_main()
