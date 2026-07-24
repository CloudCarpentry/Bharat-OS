#!/usr/bin/env python3
import subprocess
import sys
import os
import re

def check_symbols(lib_path):
    try:
        # Use nm to list symbols
        result = subprocess.run(['nm', lib_path], capture_output=True, text=True, check=True)
        lines = result.stdout.splitlines()

        syscall_symbols = []
        for line in lines:
            # Match T (global text) or W (weak) symbols
            match = re.search(r'\s+[TW]\s+(bharat_syscall)$', line)
            if match:
                syscall_symbols.append(match.group(1))

        if len(syscall_symbols) == 0:
            print(f"ERROR: No 'bharat_syscall' symbol found in {lib_path}")
            return False
        if len(syscall_symbols) > 1:
            print(f"ERROR: Duplicate 'bharat_syscall' symbols found in {lib_path}: {syscall_symbols}")
            return False

        # Check for host_syscall_stub leakage
        for line in lines:
            if "host_syscall_stub" in line:
                print(f"ERROR: Internal test symbol 'host_syscall_stub' leaked into {lib_path}")
                return False

        print(f"Symbol check passed for {lib_path}")
        return True
    except FileNotFoundError:
        print("ERROR: 'nm' tool not found.")
        return False
    except subprocess.CalledProcessError as e:
        print(f"ERROR running nm: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: check_syscall_symbols.py <library_path>")
        sys.exit(1)

    lib = sys.argv[1]
    if not os.path.exists(lib):
        print(f"File not found: {lib}")
        sys.exit(1)

    if not check_symbols(lib):
        sys.exit(1)
