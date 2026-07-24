#!/usr/bin/env python3
"""
Linter/Checker for Status and Error Codes in Bharat-OS.

Verifies:
1. No duplicate numeric assignments in kstatus_t (status.h)
2. No duplicate numeric assignments in sys_errno_t (sys_errno.h)
3. No duplicate symbolic names.
4. Ensures consistency.
"""

import os
import re
import sys

def parse_header(filepath, prefix, is_negative=False):
    """
    Parses a C header file looking for macros that define errors.
    Returns a dictionary of name -> numeric_value and numeric_value -> list of names.
    """
    if not os.path.exists(filepath):
        print(f"Error: Could not find {filepath}")
        sys.exit(1)

    name_to_val = {}
    val_to_names = {}

    with open(filepath, 'r') as f:
        for line_num, line in enumerate(f, 1):
            line = line.strip()
            if not line.startswith('#define '):
                continue

            parts = line.split()
            if len(parts) < 3:
                continue

            name = parts[1]
            if not name.startswith(prefix):
                continue

            val_str = parts[2]

            # Extract numeric value
            # Handle forms like: ((kstatus_t)-1) or 1
            match = re.search(r'-?\d+', val_str)
            if not match:
                continue

            val = int(match.group(0))
            if is_negative and val > 0:
                print(f"Warning: Expected negative value for {name} but got {val}")

            if name in name_to_val:
                print(f"Error: Duplicate symbol name '{name}' in {filepath}:{line_num}")
                sys.exit(1)

            name_to_val[name] = val
            if val not in val_to_names:
                val_to_names[val] = []
            val_to_names[val].append(name)

    return name_to_val, val_to_names

def check_duplicates(val_to_names, file_desc):
    errors = 0
    for val, names in val_to_names.items():
        if len(names) > 1:
            print(f"Error: Duplicate numeric value {val} used by multiple symbols in {file_desc}: {', '.join(names)}")
            errors += 1
    return errors

def main():
    root_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    kstatus_file = os.path.join(root_dir, 'core', 'kernel', 'include', 'kernel', 'status.h')
    sys_errno_file = os.path.join(root_dir, 'interface', 'include', 'bharat', 'uapi', 'sys_errno.h')

    print(f"Checking kernel status codes in {kstatus_file}...")
    k_name_to_val, k_val_to_names = parse_header(kstatus_file, "K_ERR_", is_negative=True)
    k_errors = check_duplicates(k_val_to_names, "core/kernel/include/kernel/status.h")

    print(f"Checking UAPI sys_errno codes in {sys_errno_file}...")
    sys_name_to_val, sys_val_to_names = parse_header(sys_errno_file, "SYS_E", is_negative=False)
    sys_errors = check_duplicates(sys_val_to_names, "interface/include/bharat/uapi/sys_errno.h")

    total_errors = k_errors + sys_errors

    if total_errors > 0:
        print(f"\nFailed: Found {total_errors} duplicate assignment errors.")
        sys.exit(1)

    print("\nSuccess: No duplicate status codes found. Consistency checks passed.")
    sys.exit(0)

if __name__ == '__main__':
    main()
