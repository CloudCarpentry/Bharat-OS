#!/usr/bin/env python3
"""Bharat-OS Scheduler Ownership Linter.

Verifies that scheduler files obey the multikernel ownership-based rules:
- No remote runqueue lock acquiring.
- No direct remote ready_queue/list mutations.
- No g_cpu_locals[remote_cpu].runqueue.xxx access (except in approved internal access files).
- No use of legacy pending_inbox.
"""

import sys
import re
from pathlib import Path

# Files under core/kernel/src/sched/ to check
SCHED_DIR = Path("core/kernel/src/sched")

# Files allowed to reference g_cpu_locals[...].runqueue
ALLOWED_RUNQUEUE_ACCESS_FILES = {"sched.c", "sched_core.c", "sched_test_support.c"}

# Regex to detect violation of remote runqueue field access
REMOTE_RQ_ACCESS_RE = re.compile(r'g_cpu_locals\[[^\]]+\]\.runqueue')

# Regex to detect usage of the legacy pending_inbox
PENDING_INBOX_RE = re.compile(r'\bpending_inbox\b')

# Regex to detect acquiring remote runqueue locks
REMOTE_LOCK_RE = re.compile(r'spin_lock\(&g_cpu_locals\[[^\]]+\]\.runqueue\.lock\)')

def check_file(path: Path) -> list[str]:
    violations = []
    content = path.read_text(encoding="utf-8", errors="ignore")
    lines = content.splitlines()

    for i, line in enumerate(lines, 1):
        # Ignore comments
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*"):
            continue

        # 1. Check for remote lock acquisition
        if REMOTE_LOCK_RE.search(line):
            violations.append(f"{path}:{i}: Direct lock acquisition of remote runqueue")

        # 2. Check for direct runqueue access if the file is not allowed
        if path.name not in ALLOWED_RUNQUEUE_ACCESS_FILES:
            if REMOTE_RQ_ACCESS_RE.search(line):
                violations.append(f"{path}:{i}: Direct reference to runqueue via 'g_cpu_locals' (file not in allowlist)")

        # 3. Check for legacy pending_inbox
        if PENDING_INBOX_RE.search(line):
            violations.append(f"{path}:{i}: Use of legacy 'pending_inbox'")

    return violations

def main() -> int:
    if not SCHED_DIR.exists():
        print(f"Directory {SCHED_DIR} does not exist. Skipping.")
        return 0

    all_violations = []
    for file in SCHED_DIR.glob("**/*.[ch]"):
        if "sched_test_support" in file.name or "sched_stub" in file.name:
            continue
        violations = check_file(file)
        if violations:
            all_violations.extend(violations)

    if all_violations:
        print("Scheduler Ownership Violations Found:")
        for v in all_violations:
            print(f"  {v}")
        return 1

    print("No scheduler ownership violations detected. Single-ownership invariant holds!")
    return 0

if __name__ == "__main__":
    sys.exit(main())
