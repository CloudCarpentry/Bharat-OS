import re
import os
import sys
import argparse
import common

SYSCALL_TABLE_PATH = "interface/include/bharat/uapi/syscall/table.def"
MANIFEST_PATH = "interface/contracts/abi/syscalls.json"

def get_all_entries():
    if not os.path.exists(SYSCALL_TABLE_PATH):
        common.report_error(f"Syscall table not found: {SYSCALL_TABLE_PATH}")
        return []

    entries = []
    with open(SYSCALL_TABLE_PATH, 'r') as f:
        for line in f:
            line = line.strip()
            # Look for SYSCALL_DEF(name, number)
            m = re.match(r'^SYSCALL_DEF\s*\(\s*([A-Za-z0-9_]+)\s*,\s*(\d+)\s*\)', line)
            if m:
                entries.append((m.group(1), int(m.group(2))))
    return entries

def validate_duplicates(entries):
    nums_seen = {}
    names_seen = {}
    success = True
    for name, num in entries:
        if num in nums_seen:
            common.report_error(f"Duplicate syscall number {num} used by {nums_seen[num]} and {name}.")
            success = False
        nums_seen[num] = name

        if name in names_seen:
            common.report_error(f"Duplicate syscall name {name} used for numbers {names_seen[name]} and {num}.")
            success = False
        names_seen[name] = num
    return success

def check_syscalls(baseline, current):
    success = True

    # 1. Existing syscalls must not be renumbered, removed, or renamed
    for number_str, name in baseline.items():
        if number_str not in current:
            common.report_error(f"Syscall {name} ({number_str}) was removed. Syscall deletions are forbidden.")
            success = False
            continue

        if current[number_str] != name:
            common.report_error(f"Syscall {number_str} was changed from {name} to {current[number_str]}. Renumbering/Renaming is forbidden.")
            success = False

    # 2. Check for duplicate numbers in current
    entries = get_all_entries()
    if not validate_duplicates(entries):
        success = False

    # 3. Check if table is sorted by number (good practice for ABI files)
    nums = [num for name, num in entries]
    if nums != sorted(nums):
         # Soft check for now
         pass

    return success

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Check syscall ABI compatibility.')
    parser.add_argument('--check', action='store_true', help='Check against baseline manifest')
    parser.add_argument('--update', action='store_true', help='Update baseline manifest')
    args = parser.parse_args()

    entries = get_all_entries()
    current = {str(num): name for name, num in entries}

    if args.update:
        if not validate_duplicates(entries):
            sys.exit(1)
        common.save_manifest(MANIFEST_PATH, current)
        print(f"Updated baseline manifest: {MANIFEST_PATH}")
        sys.exit(0)

    if args.check:
        baseline = common.load_manifest(MANIFEST_PATH)
        if not common.check_compatibility(baseline, current, "Syscall ABI", check_syscalls):
            sys.exit(1)
        print("Syscall ABI check passed.")
        sys.exit(0)

    # Default to print current
    print(current)
