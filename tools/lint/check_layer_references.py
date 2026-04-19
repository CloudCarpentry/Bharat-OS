#!/usr/bin/env python3
"""Bharat-OS layer reference linter.

Checks include-level architecture boundaries across C/C++ source and header files.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path

INCLUDE_RE = re.compile(r'^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]')

LAYER_PREFIX = {
    "kernel": "kernel/",
    "hal": "hal/",
    "arch": "arch/",
    "platform": "platform/",
    "drivers": "drivers/",
    "services": "services/",
    "stacks": "stacks/",
    "user": "user/",
    "sdk": "sdk/",
    "uapi": "uapi/",
    "lib": "lib/",
    "boot": "boot/",
    "include": "include/",
    "personalities": "personalities/",
    "tests": "tests/",
}

# Conservative policy intended to keep kernel/hal/arch/platform clean and
# user/service layers from bypassing contracts.
ALLOWED_REFS = {
    "kernel": {"kernel", "hal", "arch", "platform", "drivers", "lib", "include", "uapi", "boot"},
    "hal": {"hal", "arch", "platform", "lib", "include", "uapi", "boot"},
    "arch": {"arch", "hal", "platform", "lib", "include", "uapi", "boot"},
    "platform": {"platform", "hal", "arch", "lib", "include", "uapi", "boot", "drivers"},
    "drivers": {"drivers", "hal", "arch", "platform", "kernel", "lib", "include", "uapi"},
    "services": {"services", "stacks", "lib", "include", "uapi", "sdk", "user", "personalities"},
    "stacks": {"stacks", "services", "lib", "include", "uapi", "sdk", "user", "personalities"},
    "user": {"user", "sdk", "lib", "include", "uapi", "services", "stacks", "personalities"},
    "sdk": {"sdk", "lib", "include", "uapi", "user", "services", "stacks", "personalities"},
    "uapi": {"uapi", "include", "lib"},
    "lib": {"lib", "include", "uapi"},
    "boot": {"boot", "hal", "arch", "platform", "lib", "include", "uapi"},
    "include": set(LAYER_PREFIX.keys()) | {"other"},
    "personalities": {"personalities", "sdk", "lib", "include", "uapi", "services", "stacks", "user"},
    "tests": set(LAYER_PREFIX.keys()) | {"other"},
    "other": set(LAYER_PREFIX.keys()) | {"other"},
}

EXCLUDED_DIRS = {".git", "build", "out"}
CODE_SUFFIXES = (".c", ".h", ".cc", ".cpp", ".hpp", ".S")


@dataclass(frozen=True)
class Violation:
    file: str
    line: int
    include: str
    source_layer: str
    target_layer: str


def detect_layer(path: str) -> str:
    for layer, prefix in LAYER_PREFIX.items():
        if path.startswith(prefix):
            return layer
    return "other"


def include_target_layer(include_target: str) -> str | None:
    top = include_target.split("/", 1)[0]
    for layer, prefix in LAYER_PREFIX.items():
        if top == prefix.rstrip("/"):
            return layer
    return None


def iter_code_files(repo_root: Path):
    for root, dirs, files in os.walk(repo_root):
        dirs[:] = [d for d in dirs if d not in EXCLUDED_DIRS and not d.startswith("build-")]
        for name in files:
            if name.endswith(CODE_SUFFIXES):
                p = Path(root) / name
                rel = p.relative_to(repo_root).as_posix()
                yield p, rel


def scan(repo_root: Path) -> tuple[list[Violation], int]:
    violations: list[Violation] = []
    file_count = 0

    for abspath, relpath in iter_code_files(repo_root):
        file_count += 1
        src_layer = detect_layer(relpath)

        try:
            with abspath.open("r", encoding="utf-8", errors="ignore") as f:
                for ln, line in enumerate(f, start=1):
                    m = INCLUDE_RE.match(line)
                    if not m:
                        continue

                    target = include_target_layer(m.group(1))
                    if target is None:
                        continue

                    if target not in ALLOWED_REFS.get(src_layer, set()):
                        violations.append(
                            Violation(relpath, ln, m.group(1), src_layer, target)
                        )
        except OSError:
            continue

    return violations, file_count


def write_report(path: Path, violations: list[Violation], files_scanned: int) -> None:
    by_pair = Counter((v.source_layer, v.target_layer) for v in violations)
    examples: dict[tuple[str, str], list[Violation]] = defaultdict(list)
    for v in violations:
        key = (v.source_layer, v.target_layer)
        if len(examples[key]) < 8:
            examples[key].append(v)

    lines = []
    lines.append("# Layer Reference Gap Analysis (Generated)")
    lines.append("")
    lines.append(f"- Files scanned: **{files_scanned}**")
    lines.append(f"- Violations found: **{len(violations)}**")
    lines.append("")
    lines.append("## Violation Summary")
    lines.append("")
    if not violations:
        lines.append("No layer-reference violations detected.")
    else:
        lines.append("| Source Layer | Target Layer | Count |")
        lines.append("|---|---:|---:|")
        for (src, dst), count in by_pair.most_common():
            lines.append(f"| `{src}` | `{dst}` | {count} |")

        lines.append("")
        lines.append("## Representative Violations")
        lines.append("")
        for key, _count in by_pair.most_common():
            src, dst = key
            lines.append(f"### `{src}` -> `{dst}`")
            for v in examples[key]:
                lines.append(f"- `{v.file}:{v.line}` includes `{v.include}`")
            lines.append("")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Check include-layer architecture references")
    parser.add_argument("--report", type=str, help="Optional path to write markdown report")
    parser.add_argument("--strict", action="store_true", help="Return non-zero when violations are present")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    violations, files_scanned = scan(repo_root)

    print(f"Scanned {files_scanned} code/header files")
    print(f"Found {len(violations)} layer-reference violations")

    if args.report:
        report_path = (repo_root / args.report).resolve()
        report_path.parent.mkdir(parents=True, exist_ok=True)
        write_report(report_path, violations, files_scanned)
        print(f"Report written to: {report_path}")

    if args.strict and violations:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
