#!/usr/bin/env python3
import json
import sys
import re
import os

def parse_header(header_path):
    syscalls = {}
    count_val = None
    pattern = re.compile(r'#define\s+(BH_SYS_[A-Z0-9_]+)\s+(\d+)')
    count_pattern = re.compile(r'#define\s+BH_SYSCALL_COUNT\s+(\d+)')
    with open(header_path, 'r') as f:
        for line in f:
            match = pattern.search(line)
            if match:
                name = match.group(1)
                number = int(match.group(2))
                syscalls[number] = name

            count_match = count_pattern.search(line)
            if count_match:
                count_val = int(count_match.group(1))

    return syscalls, count_val

def check_abi(manifest_path, header_path):
    with open(manifest_path, 'r') as f:
        manifest = json.load(f)

    header_syscalls, header_count = parse_header(header_path)
    manifest_syscalls = {s['number']: s for s in manifest['syscalls']}

    errors = []

    # Check for removals or reorders
    for m_num, m_entry in manifest_syscalls.items():
        m_name = m_entry['name']
        m_status = m_entry.get('status', 'stable')

        if m_status in ['stable', 'deprecated']:
            if m_num not in header_syscalls:
                errors.append(f"ERROR: Syscall {m_name} ({m_num}) [{m_status}] present in manifest but missing from header.")
            elif header_syscalls[m_num] != m_name:
                errors.append(f"ERROR: Syscall {m_num} name mismatch. Manifest: {m_name}, Header: {header_syscalls[m_num]}")

        if m_status == 'reserved':
            if m_num in header_syscalls:
                errors.append(f"ERROR: Reserved syscall number {m_num} ({m_name}) is being used in header by {header_syscalls[m_num]}")

    # Check for unexpected changes in existing numbers in header
    max_manifest_num = max(manifest_syscalls.keys()) if manifest_syscalls else -1
    for h_num, h_name in header_syscalls.items():
        if h_num in manifest_syscalls:
            m_entry = manifest_syscalls[h_num]
            if m_entry['name'] != h_name:
                errors.append(f"ERROR: Syscall {h_num} redefines {m_entry['name']} to {h_name}")
        else:
            # New syscall - allowed if number is higher than max in manifest
            if h_num <= max_manifest_num:
                 errors.append(f"ERROR: New syscall {h_name} ({h_num}) uses a number already reserved or lower than max manifest number.")

    # Validate BH_SYSCALL_COUNT
    if header_count is not None:
        expected_count = max(header_syscalls.keys()) + 1 if header_syscalls else 0
        if header_count != expected_count:
            errors.append(f"ERROR: BH_SYSCALL_COUNT mismatch. Header: {header_count}, Expected: {expected_count}")

    if errors:
        for err in errors:
            print(err)
        return False

    print("Syscall ABI check passed.")
    return True

if __name__ == "__main__":
    manifest = "contracts/abi/syscall_manifest.json"
    header = "interface/include/bharat/uapi/syscall/bh_syscall_numbers.h"

    if not os.path.exists(manifest) or not os.path.exists(header):
        print(f"Paths missing: {manifest} or {header}")
        sys.exit(1)

    if not check_abi(manifest, header):
        sys.exit(1)
