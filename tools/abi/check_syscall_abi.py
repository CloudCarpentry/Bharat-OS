#!/usr/bin/env python3
import json
import sys
import re
import os

def parse_header(header_path):
    syscalls = {}
    pattern = re.compile(r'#define\s+(BH_SYS_[A-Z0-9_]+)\s+(\d+)')
    with open(header_path, 'r') as f:
        for line in f:
            match = pattern.search(line)
            if match:
                name = match.group(1)
                number = int(match.group(2))
                syscalls[number] = name
    return syscalls

def check_abi(manifest_path, header_path):
    with open(manifest_path, 'r') as f:
        manifest = json.load(f)

    header_syscalls = parse_header(header_path)
    manifest_syscalls = {s['number']: s['name'] for s in manifest['syscalls']}

    errors = []

    # Check for removals or reorders
    for m_num, m_name in manifest_syscalls.items():
        if m_num not in header_syscalls:
            errors.append(f"ERROR: Syscall {m_name} ({m_num}) present in manifest but missing from header.")
        elif header_syscalls[m_num] != m_name:
            errors.append(f"ERROR: Syscall {m_num} name mismatch. Manifest: {m_name}, Header: {header_syscalls[m_num]}")

    # Check for unexpected changes in existing numbers in header
    for h_num, h_name in header_syscalls.items():
        if h_num in manifest_syscalls:
            if manifest_syscalls[h_num] != h_name:
                # Already caught above but for completeness
                pass
        else:
            # New syscall - allowed if number is higher than max in manifest
            max_manifest_num = max(manifest_syscalls.keys()) if manifest_syscalls else -1
            if h_num <= max_manifest_num:
                 errors.append(f"ERROR: New syscall {h_name} ({h_num}) uses a number already reserved or lower than max manifest number.")

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
