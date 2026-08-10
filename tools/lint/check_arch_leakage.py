#!/usr/bin/env python3

import os
import re
import sys

# Directories to scan for leakage
SCAN_DIRS = [
    'core/kernel/src',
    'core/kernel/include',
    'core/hal/common',
    'core/lib'
]

# Patterns that indicate architecture leakage
LEAKAGE_PATTERNS = [
    re.compile(r'__x86_64__'),
    re.compile(r'__i386__'),
    re.compile(r'__aarch64__'),
    re.compile(r'__arm__'),
    re.compile(r'__riscv'),
    re.compile(r'__riscv_xlen'),
    re.compile(r'\binb\s*\('),
    re.compile(r'\boutb\s*\('),
    re.compile(r'ARCH_CPU_FEAT_ARM'),
    re.compile(r'ARCH_CPU_FEAT_RISCV'),
    re.compile(r'#include\s+["<](arch/.*|hal/x86_64/.*|hal/aarch64/.*|hal/arm/.*|hal/riscv.*/.*|hal/i386/.*)[">]')
]

def load_allowlist(allowlist_path):
    allowlist = set()
    if os.path.exists(allowlist_path):
        with open(allowlist_path, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    allowlist.add(line)
    return allowlist

# Excluded files (allowlist)
EXCLUDED_FILES = [
    'core/kernel/src/kernel_boot.c',
    'core/kernel/src/init/init_bootstrap.c',
    # Allowed to use arch_cpu_caps for common definitions
    'core/kernel/include/arch/arch_cpu_caps.h'
]

def check_file(filepath):
    violations = []
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                for pattern in LEAKAGE_PATTERNS:
                    if pattern.search(line):
                        violations.append(f"{filepath}:{line_num}: {line.strip()}")
    except UnicodeDecodeError:
        pass # Skip binary files if any
    return violations

def main():
    all_violations = []

    allowlist = load_allowlist('tools/lint/baselines/arch_leakage.allowlist')

    for scan_dir in SCAN_DIRS:
        if not os.path.exists(scan_dir):
            continue

        for root, _, files in os.walk(scan_dir):
            for file in files:
                if not (file.endswith('.c') or file.endswith('.h')):
                    continue

                filepath = os.path.join(root, file)

                # Check exclusions
                if filepath in EXCLUDED_FILES or filepath in allowlist:
                    continue

                # Check for syscall entry exclusions
                if 'syscall/entry' in filepath:
                    continue

                violations = check_file(filepath)
                if violations:
                    all_violations.extend(violations)

    if all_violations:
        print(f"Found {len(all_violations)} architecture leakage violations:")
        for v in all_violations:
            print(f"  {v}")
        sys.exit(1)
    else:
        print("No architecture leakage found.")
        sys.exit(0)

if __name__ == '__main__':
    main()
