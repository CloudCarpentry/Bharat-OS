#!/usr/bin/env python3
"""Verify that C-generated offsets are the CPU-context layout authority."""

import pathlib
import re
import sys


REQUIRED = {
    *(f"BH_CTX_REG{i}_OFF" for i in range(13)),
    "BH_CTX_PC_OFF",
    "BH_CTX_SP_OFF",
    "BH_CTX_EXT_OFF",
    "BH_CTX_SIZE",
}
FORBIDDEN_CONTEXT_OFFSETS = re.compile(
    r"(?:\b(?:128|136|400)\s*\(%r(?:di|si)\)|"
    r"\[(?:x|r)[01],\s*#(?:128|136)\]|"
    r"\b(?:16|17)\s*\*\s*REGBYTES\(a[01]\))"
)


def fail(message: str) -> None:
    raise SystemExit(f"context ABI check failed: {message}")


def main() -> None:
    if len(sys.argv) < 3:
        fail(f"usage: {sys.argv[0]} <generated_offsets> <asm> [<asm> ...]")

    generated = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
    symbols = set(
        re.findall(r"^#define\s+(BH_[A-Z0-9_]+)\s+", generated, re.MULTILINE)
    )
    missing = sorted(REQUIRED - symbols)
    if missing:
        fail(f"generated header lacks: {', '.join(missing)}")

    for source_name in sys.argv[2:]:
        source = pathlib.Path(source_name)
        text = source.read_text(encoding="utf-8")
        if '#include "trap_offsets.inc"' not in text:
            fail(f"{source} does not include generated offsets")
        for line_number, line in enumerate(text.splitlines(), 1):
            if FORBIDDEN_CONTEXT_OFFSETS.search(line):
                fail(f"{source}:{line_number} duplicates a CPU-context offset")

    print(f"PASS: verified generated CPU-context ABI across {len(sys.argv) - 2} assembly sources")


if __name__ == "__main__":
    main()
