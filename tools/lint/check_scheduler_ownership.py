#!/usr/bin/env python3
"""Bharat-OS Scheduler Ownership Linter.

Verifies that scheduler files obey the multikernel ownership-based rules:
- No remote runqueue lock acquiring.
- No direct remote ready_queue/list mutations.
- No remote runqueue access (except for transport rings and snapshot/load/initialization helpers).
"""

import sys
import re
from pathlib import Path

# Files under core/kernel/src/sched/ to check
SCHED_DIR = Path("core/kernel/src/sched")

# Local CPU identifiers that are approved for full local runqueue access
LOCAL_CPU_VARS = {
    "core", "current_core", "current_cpu", "core_id", "current_core_id",
    "hal_cpu_get_id()", "sched_clamp_core(hal_cpu_get_id())",
    "saved_cpu", "creation_core_id"
}

# Forbidden fields on any remote runqueue object
FORBIDDEN_FIELDS = {
    "ready_queue", "ready_bitmap", "cfs_runqueue", "edf_runqueue",
    "current_thread", "idle_thread", "sleeping_list", "blocked_list",
    "reap_head", "reap_tail", "free_thread_head", "free_process_head",
    "runnable_count", "lock"
}

# Regex to detect remote alias initialization
ALIAS_RE = re.compile(r'sched_rq_t\s*\*(\w+)\s*=\s*&g_cpu_locals\[([^\]]+)\]\.runqueue')

def check_file(path: Path) -> list[str]:
    violations = []
    content = path.read_text(encoding="utf-8", errors="ignore")
    lines = content.splitlines()

    remote_aliases = set()

    for i, line in enumerate(lines, 1):
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("/*") or stripped.startswith("*"):
            continue

        # Check for function scope end
        if stripped == "}":
            remote_aliases.clear()
            continue

        # Check for alias declaration/shadowing
        match_alias_decl = ALIAS_RE.search(line)
        if match_alias_decl:
            alias_name = match_alias_decl.group(1)
            cpu_var = match_alias_decl.group(2).strip()
            if cpu_var not in LOCAL_CPU_VARS:
                remote_aliases.add(alias_name)
            else:
                remote_aliases.discard(alias_name)

        # Check for direct g_cpu_locals[remote_cpu] forbidden field access
        match_g = re.search(r'g_cpu_locals\[([^\]]+)\]\.runqueue\.(\w+)', line)
        if match_g:
            cpu_var, field = match_g.group(1).strip(), match_g.group(2)
            if cpu_var not in LOCAL_CPU_VARS:
                if field in FORBIDDEN_FIELDS:
                    violations.append(
                        f"{path}:{i}: Direct forbidden remote field access '{field}' on 'g_cpu_locals[{cpu_var}]'"
                    )

        # Check for remote lock acquisition via spin_lock
        if "spin_lock" in line or "spin_unlock" in line:
            match_lock = re.search(r'g_cpu_locals\[([^\]]+)\]\.runqueue', line)
            if match_lock:
                cpu_var = match_lock.group(1).strip()
                if cpu_var not in LOCAL_CPU_VARS:
                    violations.append(
                        f"{path}:{i}: Attempt to acquire or release remote runqueue lock directly"
                    )

        # Check for ALIAS->field forbidden remote access
        for alias in remote_aliases:
            match_alias = re.search(r'\b' + re.escape(alias) + r'->(\w+)', line)
            if match_alias:
                field = match_alias.group(1)
                if field in FORBIDDEN_FIELDS:
                    violations.append(
                        f"{path}:{i}: Forbidden remote field access '{field}' via remote alias '{alias}'"
                    )

        # Prevent use of legacy pending_inbox
        if "pending_inbox" in line:
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
