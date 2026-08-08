#!/usr/bin/env python3
import sys
import re

if len(sys.argv) != 3:
    print(f"Usage: {sys.argv[0]} <input.s> <output.h>")
    sys.exit(1)

input_file = sys.argv[1]
output_file = sys.argv[2]

pattern = re.compile(r'->\s+([A-Za-z0-9_]+)\s+([^\s]+)\s+.*')

defines = []

with open(input_file, 'r') as f:
    for line in f:
        line = line.strip()
        match = pattern.search(line)
        if match:
            sym, val = match.groups()
            # Remove any prefix like $ or # added by the assembler
            val = re.sub(r'^[$#]', '', val)
            defines.append(f'#define {sym} {val}')

with open(output_file, 'w') as f:
    f.write('#ifndef BHARAT_GENERATED_ASM_OFFSETS_H\n')
    f.write('#define BHARAT_GENERATED_ASM_OFFSETS_H\n\n')
    for d in defines:
        f.write(d + '\n')
    f.write('\n#endif\n')
