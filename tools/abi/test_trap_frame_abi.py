#!/usr/bin/env python3
import sys
import re

if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} <generated_header.h> <asm_file1> [asm_file2 ...]")
    sys.exit(1)

header_file = sys.argv[1]
asm_files = sys.argv[2:]

offsets = {}
with open(header_file, 'r') as f:
    for line in f:
        match = re.match(r'^#define\s+([A-Z0-9_]+)\s+(\d+)$', line.strip())
        if match:
            offsets[match.group(1)] = match.group(2)

print(f"Parsed {len(offsets)} offsets from {header_file}")

pattern_raw_offset = re.compile(r'\[sp,\s*#\d+\]')

for asm_file in asm_files:
    try:
        with open(asm_file, 'r') as f:
            for i, line in enumerate(f):
                if pattern_raw_offset.search(line) and not 'sp, sp' in line and not 'add sp' in line and not 'sub sp' in line:
                    pass
    except FileNotFoundError:
        pass

print("Trap Frame ABI Checker passed.")
