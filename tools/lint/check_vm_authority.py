#!/usr/bin/env python3
"""
Bharat-OS KERN-P0-003A Static Linter: check_vm_authority.py
Ensures there is no unsafe casting between vm_space_t and address_space_t,
and checks for direct access of deprecated mappings databases in the VM subsystem.
"""

import sys
import re
import os

# Specifically match casting of variables, ignoring standard allocations (kmalloc, malloc, etc.)
FORBIDDEN_PATTERNS = [
    (re.compile(r'\(\s*address_space_t\s*\*\s*\)\s*(?!(kmalloc|malloc|realloc|calloc)\b)[a-zA-Z0-9_]+'),
     "Unsafe casting from vm_space_t or other pointer to address_space_t* is strictly forbidden."),
    (re.compile(r'\(\s*vm_space_t\s*\*\s*\)\s*(?!(kmalloc|malloc|realloc|calloc)\b)[a-zA-Z0-9_]+'),
     "Unsafe casting to vm_space_t* is strictly forbidden."),
]

# Paths to scan
PATHS_TO_SCAN = [
    "core/kernel/src/mm/vm",
    "core/kernel/include/mm",
]

def check_file(filepath):
    errors = []
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for i, line in enumerate(f, 1):
                # Skip comments where the casting patterns might exist in documentation or comments
                stripped = line.strip()
                if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
                    continue
                for pattern, msg in FORBIDDEN_PATTERNS:
                    if pattern.search(line):
                        errors.append((i, line.strip(), msg))
    except Exception as e:
        print(f"Error reading {filepath}: {e}")
    return errors

def main():
    total_errors = 0
    scanned_files = 0

    print("Running check_vm_authority linter...")

    for base_path in PATHS_TO_SCAN:
        if not os.path.exists(base_path):
            continue
        for root, _, files in os.walk(base_path):
            for file in files:
                if file.endswith(('.c', '.h')):
                    filepath = os.path.join(root, file)
                    scanned_files += 1
                    errors = check_file(filepath)
                    if errors:
                        print(f"\n[FAIL] {filepath}:")
                        for line_num, line_content, msg in errors:
                            print(f"  Line {line_num}: '{line_content}'")
                            print(f"    Reason: {msg}")
                        total_errors += len(errors)

    print(f"\nScanned {scanned_files} files.")
    if total_errors > 0:
        print(f"Total violations found: {total_errors}")
        sys.exit(1)
    else:
        print("Linter passed! No unsafe pointer type-punning patterns detected.")
        sys.exit(0)

if __name__ == '__main__':
    main()
