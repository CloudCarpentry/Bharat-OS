import re
import os
import sys
import common

SYSCALL_TABLE_PATH = "include/bharat/uapi/syscall_table.def"

def generate_syscall_manifest():
    if not os.path.exists(SYSCALL_TABLE_PATH):
        common.report_error(f"Syscall table not found: {SYSCALL_TABLE_PATH}")
        return {}

    manifest = {}
    with open(SYSCALL_TABLE_PATH, 'r') as f:
        for line in f:
            line = line.strip()
            # Look for SYSCALL_DEF(name, number)
            m = re.match(r'^SYSCALL_DEF\s*\(\s*([A-Za-z0-9_]+)\s*,\s*(\d+)\s*\)', line)
            if m:
                name = m.group(1)
                number = int(m.group(2))
                manifest[str(number)] = name

    return manifest

def check_syscalls(baseline, current):
    if baseline is None:
        return False

    success = True

    # 1. Existing syscalls must not be renumbered or removed
    for number_str, name in baseline.items():
        if number_str not in current:
            common.report_error(f"Syscall {name} ({number_str}) was removed. Syscall deletions are forbidden.")
            success = False
            continue

        if current[number_str] != name:
            common.report_error(f"Syscall {number_str} was renumbered from {name} to {current[number_str]}. Renumbering is forbidden.")
            success = False

    # 2. Check for duplicate numbers in current
    # Our dict comprehension prevents duplicate keys, but let's check values (names)
    names_seen = {}
    for num, name in current.items():
        if name in names_seen:
            common.report_error(f"Syscall name {name} has duplicate entries: {names_seen[name]} and {num}.")
            success = False
        names_seen[name] = num

    return success

if __name__ == "__main__":
    curr = generate_syscall_manifest()
    print(curr)
