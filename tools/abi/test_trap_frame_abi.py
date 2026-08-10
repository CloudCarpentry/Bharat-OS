#!/usr/bin/env python3
"""Verify that C-generated trap offsets are the only frame-layout authority."""

import pathlib
import re
import sys

REQUIRED = {
    "BH_TF_GPR0_OFF", "BH_TF_SP_OFF", "BH_TF_PC_OFF",
    "BH_TF_CAUSE_OFF", "BH_TF_STATUS_OFF", "BH_TF_TYPE_OFF",
    "BH_TF_FROM_USER_OFF", "BH_TF_SIZE", "BH_TF_STACK_SIZE",
}
NUMERIC_FRAME_ACCESS = re.compile(r"(?:\[sp,\s*#|\b)(?:248|256|264|272|280|284|288|296)\s*(?:\]|\(%rsp\))")


def fail(message: str) -> None:
    raise SystemExit(f"trap ABI check failed: {message}")


def main() -> None:
    if len(sys.argv) < 3:
        fail(f"usage: {sys.argv[0]} <trap_offsets.inc> <asm> [<asm> ...]")

    header = pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")
    symbols = set(re.findall(r"^#define\s+(BH_[A-Z0-9_]+)\s+", header, re.MULTILINE))
    missing = sorted(REQUIRED - symbols)
    if missing:
        fail(f"generated header lacks: {', '.join(missing)}")

    sources = {}
    for name in sys.argv[2:]:
        path = pathlib.Path(name)
        text = path.read_text(encoding="utf-8")
        sources[path.name + ':' + str(path.parent)] = text
        if '#include "trap_offsets.inc"' not in text:
            fail(f"{path} does not include generated offsets")
        for lineno, line in enumerate(text.splitlines(), 1):
            if NUMERIC_FRAME_ACCESS.search(line) and not line.lstrip().startswith(('#', '/*', '*')):
                fail(f"{path}:{lineno} contains a duplicated numeric frame offset")

    joined = "\n".join(sources.values())
    rv_sources = [text for key, text in sources.items() if "riscv" in key]
    for text in rv_sources:
        if "sw t0, BH_TF_TYPE_OFF(sp)" not in text or "sw t0, BH_TF_FROM_USER_OFF(sp)" not in text:
            fail("RISC-V type/from_user fields are not written as fixed-width u32 values")

    arm32 = next((text for key, text in sources.items() if "arm32" in key), "")
    if arm32 and ("ARM32_VECTOR irq_handler" not in arm32 or "irq_handler:\n    b svc_handler" in arm32):
        fail("ARM32 IRQ does not have an independent vector classification")

    x86 = next((text for key, text in sources.items() if "x86_64" in key), "")
    if x86 and (x86.count("swapgs") != 2 or ".L_kernel_sp" not in x86):
        fail("x86 CPL-aware stack decoding and balanced swapgs are missing")

    print(f"PASS: verified generated trap ABI across {len(sources)} assembly entries")


if __name__ == "__main__":
    main()
