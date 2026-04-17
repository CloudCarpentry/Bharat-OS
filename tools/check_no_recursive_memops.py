#!/usr/bin/env python3
"""
tools/check_no_recursive_memops.py

Checks a compiled kernel ELF file for recursive calls in memory operations
like memset, memcpy, and memmove using llvm-objdump or objdump.
"""

import sys
import subprocess
import shutil
import re

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <path_to_kernel_elf>")
        sys.exit(1)

    elf_path = sys.argv[1]

    # Find objdump
    objdump_cmd = shutil.which("llvm-objdump")
    if not objdump_cmd:
        objdump_cmd = shutil.which("objdump")

    if not objdump_cmd:
        print("Error: Neither llvm-objdump nor objdump was found in PATH.")
        sys.exit(1)

    try:
        # Disassemble the elf file
        result = subprocess.run(
            [objdump_cmd, "-d", elf_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )
    except subprocess.CalledProcessError as e:
        print(f"Error running {objdump_cmd}: {e.stderr}")
        sys.exit(1)

    lines = result.stdout.splitlines()

    # The functions we want to ensure do not call themselves or other libc-style symbols recursively
    targets = ["memset", "memcpy", "memmove", "bharat_memset_scalar", "bharat_memcpy_scalar", "bharat_memmove_scalar"]

    # Track the current function being disassembled
    current_func = None

    # Regex to match function headers like `<memset>:` or `0000000000001234 <memset>:`
    func_header_re = re.compile(r'^[0-9a-fA-F]+\s+<([a-zA-Z0-9_]+)>:$')

    # Regex to match call instructions like `callq  400123 <memset>` or `bl 400123 <memcpy>`
    call_re = re.compile(r'<\s*([a-zA-Z0-9_]+)\s*(?:[-+]\s*0x[0-9a-fA-F]+)?\s*>')

    errors_found = False

    for line in lines:
        header_match = func_header_re.match(line.strip())
        if header_match:
            current_func = header_match.group(1)
            continue

        if current_func in targets:
            # Check if this line is a call instruction or branch
            # We look for a call to the same function name.
            # E.g. "bl 0x1234 <memset>"
            if "call" in line or "bl " in line or "b " in line or "jal " in line or "jmp " in line:
                call_match = call_re.search(line)
                if call_match:
                    called_func = call_match.group(1)
                    if called_func == current_func:
                        print(f"ERROR: Recursive call detected in {current_func}!")
                        print(f"  Line: {line.strip()}")
                        errors_found = True

    if errors_found:
        print("Validation FAILED: Recursive memops detected.")
        sys.exit(1)

    print("Validation PASSED: No recursive memops found.")
    sys.exit(0)

if __name__ == "__main__":
    main()
