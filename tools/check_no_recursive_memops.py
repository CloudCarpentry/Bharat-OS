#!/usr/bin/env python3
"""
tools/check_no_recursive_memops.py

Rejects recursive/cross-layer memop dependencies and unexpected unresolved
compiler memory helpers in a compiled kernel ELF.
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

    memops = {
        "memcpy", "memset", "memmove", "hal_memcpy", "hal_memset", "hal_memmove",
        "hal_memcpy_scalar", "hal_memset_scalar", "hal_memmove_scalar", "hal_memset_raw",
        "bharat_memcpy_scalar", "bharat_memset_scalar", "bharat_memmove_scalar",
    }
    allowed_edges = {
        "memcpy": {"hal_memcpy"}, "memset": {"hal_memset"}, "memmove": {"hal_memmove"},
        "hal_memcpy": {"hal_memcpy_scalar", "hal_memcpy_fast_string", "hal_memcpy_gpr_bulk"},
        "hal_memset": {"hal_memset_scalar", "hal_memset_fast_string", "hal_memset_gpr_bulk"},
        "hal_memmove": {"hal_memmove_scalar"},
    }

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

        if current_func in memops:
            # Check if this line is a call instruction or branch
            # We look for a call to the same function name.
            # E.g. "bl 0x1234 <memset>"
            if "call" in line or "bl " in line or "b " in line or "jal " in line or "jmp " in line:
                call_match = call_re.search(line)
                if call_match:
                    called_func = call_match.group(1)
                    # Intra-function control-flow labels are printed as
                    # <function+0xoffset>; they are not symbol calls.
                    if called_func == current_func and f"<{current_func}+" in line:
                        continue
                    if called_func in memops and called_func not in allowed_edges.get(current_func, set()):
                        print(f"ERROR: Forbidden memop dependency {current_func} -> {called_func}")
                        print(f"  Line: {line.strip()}")
                        errors_found = True

    nm_cmd = shutil.which("llvm-nm") or shutil.which("nm")
    if not nm_cmd:
        print("Error: Neither llvm-nm nor nm was found in PATH.")
        sys.exit(1)
    nm = subprocess.run([nm_cmd, "-u", elf_path], capture_output=True, text=True, check=True)
    forbidden_undefined = re.compile(r"(?:^|\s)(memcpy|memset|memmove|__aeabi_mem\w*|__udiv\w*|__atomic_\w*)$")
    for line in nm.stdout.splitlines():
        if forbidden_undefined.search(line.strip()):
            print(f"ERROR: Unexpected undefined freestanding helper: {line.strip()}")
            errors_found = True

    if errors_found:
        print("Validation FAILED: Recursive memops detected.")
        sys.exit(1)

    print("Validation PASSED: No recursive memops found.")
    sys.exit(0)

if __name__ == "__main__":
    main()
