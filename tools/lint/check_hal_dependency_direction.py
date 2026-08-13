#!/usr/bin/env python3
"""Enforce HAL-common and library dependency-direction closure."""
from __future__ import annotations
import argparse
import re
from pathlib import Path

INCLUDE = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)

def violations(root: Path) -> list[str]:
    found: list[str] = []
    groups = ((root / "core/hal/common", ("arch/", "kernel/src/", "core/arch/", "core/kernel/src/")),
              (root / "core/lib", ("hal/hal_internal.h", "core/hal/common/", "kernel/src/", "core/kernel/src/")))
    for base, forbidden in groups:
        if not base.exists():
            continue
        for path in sorted(p for p in base.rglob("*") if p.suffix in {".c", ".h", ".S"}):
            text = path.read_text(encoding="utf-8", errors="ignore")
            for match in INCLUDE.finditer(text):
                target = match.group(1)
                if target.startswith(forbidden):
                    line = text.count("\n", 0, match.start()) + 1
                    found.append(f"{path.relative_to(root)}:{line}: forbidden include {target}")
    cmake = root / "core/hal/common/CMakeLists.txt"
    if cmake.exists():
        for line_no, line in enumerate(cmake.read_text(encoding="utf-8").splitlines(), 1):
            if "core/arch/" in line or "core/kernel/src/" in line:
                found.append(f"{cmake.relative_to(root)}:{line_no}: forbidden private path")
    return found

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    found = violations(parser.parse_args().root.resolve())
    for item in found:
        print(item)
    print(f"HAL dependency-direction violations: {len(found)}")
    return 1 if found else 0

if __name__ == "__main__":
    raise SystemExit(main())
