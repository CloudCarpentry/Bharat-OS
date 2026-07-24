#!/usr/bin/env python3
"""Bharat-OS Scheduler Ownership Linter.

Verifies that scheduler files obey the multikernel ownership-based rules:
- No remote runqueue lock acquiring.
- No direct remote ready_queue/list mutations.
- No g_cpu_locals[remote_cpu].runqueue.xxx access.
- No use of legacy pending_inbox.
"""

import sys
import re
from pathlib import Path

# Files under core/kernel/src/sched/ to check
SCHED_DIR = Path("core/kernel/src/sched")

# Regex to detect violation of remote runqueue field access
REMOTE_RQ_ACCESS_RE = re.compile(r'g_cpu_locals\[[^\]]+\]\.runqueue\.(ready_queue|ready_bitmap|cfs_runqueue|edf_runqueue|sleeping_list|blocked_list|lock|runnable_count|free_thread_head|threads|processes|bootstrap_threads)')

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

        # 2. Check for remote runqueue field access (except if the index is exactly local core)
        if REMOTE_RQ_ACCESS_RE.search(line):
            match = re.search(r'g_cpu_locals\[([^\]]+)\]\.runqueue', line)
            if match:
                index = match.group(1).strip()
                if index not in ("core", "core_id", "current_core", "current_cpu", "cpu_id", "hal_cpu_get_id()", "creation_core", "bound_core", "home_core"):
                    violations.append(f"{path}:{i}: Direct access to remote runqueue fields ('{match.group(0)}')")

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
