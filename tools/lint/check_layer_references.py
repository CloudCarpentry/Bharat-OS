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
SYMBOL_LEAKAGE_RE = re.compile(r'\b(fsd_|netmgr_|devmgr_)[a-zA-Z0-9_]+')

LAYER_PREFIXES = {
    "kernel": ("kernel/",),
    "hal": ("hal/",),
    "arch": ("arch/",),
    "platform": ("platform/",),
    "drivers": ("drivers/",),
    "services": ("services/",),
    "stacks": ("stacks/",),
    "user": ("experience/user/", "user/"),
    "sdk": ("sdk/",),
    "uapi": ("interface/uapi/", "uapi/"),
    "lib": ("lib/",),
    "boot": ("boot/",),
    "include": ("interface/include/", "include/"),
    "personalities": ("personalities/",),
    "tests": ("tests/",),
}

# Policy keeps kernel-side layers freestanding and prevents upward dependencies.
ALLOWED_REFS = {
    "kernel": {"kernel", "hal", "arch", "platform", "include", "uapi", "boot"},
    "hal": {"hal", "arch", "platform", "include", "uapi", "boot"},
    "arch": {"arch", "hal", "platform", "include", "uapi", "boot"},
    "platform": {"platform", "hal", "arch", "include", "uapi", "boot", "drivers"},
    "drivers": {"drivers", "hal", "arch", "platform", "kernel", "lib", "include", "uapi"},
    "services": {"services", "stacks", "lib", "include", "uapi", "sdk", "user", "personalities"},
    "stacks": {"stacks", "services", "lib", "include", "uapi", "sdk", "user", "personalities"},
    "user": {"user", "sdk", "lib", "include", "uapi", "services", "stacks", "personalities"},
    "sdk": {"sdk", "lib", "include", "uapi", "user", "personalities"},
    "uapi": {"uapi", "include", "lib"},
    "lib": {"lib", "include", "uapi"},
    "boot": {"boot", "hal", "arch", "platform", "include", "uapi"},
    "include": set(LAYER_PREFIXES.keys()) | {"other"},
    "personalities": {"personalities", "sdk", "lib", "include", "uapi", "services", "stacks", "user"},
    "tests": set(LAYER_PREFIXES.keys()) | {"other"},
    "other": set(LAYER_PREFIXES.keys()) | {"other"},
}

FREESTANDING_LAYERS = {"kernel", "hal", "arch", "boot", "platform"}
FORBIDDEN_HOSTED_HEADERS = {"stdio.h", "stdlib.h", "string.h"}

EXCLUDED_DIRS = {".git", "build", "out"}
CODE_SUFFIXES = (".c", ".h", ".cc", ".cpp", ".hpp", ".S")


@dataclass(frozen=True)
class Violation:
    file: str
    line: int
    include: str
    source_layer: str
    target_layer: str
    rule: str = "layer-reference"

    def key(self) -> str:
        return (
            f"{self.rule}|{self.file}|{self.line}|{self.source_layer}|"
            f"{self.target_layer}|{self.include}"
        )


def detect_layer(path: str) -> str:
    for layer, prefixes in LAYER_PREFIXES.items():
        if any(path.startswith(prefix) for prefix in prefixes):
            return layer
    return "other"


def include_target_layer(repo_root: Path, source_layer: str, include_target: str) -> str | None:
    top = include_target.split("/", 1)[0]
    for layer, prefixes in LAYER_PREFIXES.items():
        if not any(top == prefix.rstrip("/") for prefix in prefixes):
            continue

        # Treat kernel-private headers (kernel/include/lib/*) as kernel internals,
        # not top-level hosted /lib dependencies.
        if (
            layer == "lib"
            and source_layer in FREESTANDING_LAYERS
            and (repo_root / "kernel" / "include" / include_target).exists()
        ):
            return "kernel"

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
                    # Symbol leakage check for lib/sdk
                    if src_layer in {"lib", "sdk"}:
                        for match in SYMBOL_LEAKAGE_RE.finditer(line):
                            violations.append(
                                Violation(
                                    relpath,
                                    ln,
                                    match.group(0),
                                    src_layer,
                                    "service-symbol",
                                    rule="symbol-leakage",
                                )
                            )

                    m = INCLUDE_RE.match(line)
                    if not m:
                        continue

                    include_target = m.group(1)
                    if src_layer in FREESTANDING_LAYERS and include_target in FORBIDDEN_HOSTED_HEADERS:
                        violations.append(
                            Violation(
                                relpath,
                                ln,
                                include_target,
                                src_layer,
                                "hosted-header",
                                rule="freestanding-header",
                            )
                        )
                    else:
                        target = include_target_layer(repo_root, src_layer, include_target)
                        if target is not None and target not in ALLOWED_REFS.get(src_layer, set()):
                            violations.append(
                                Violation(relpath, ln, include_target, src_layer, target)
                            )
        except OSError:
            continue

    return violations, file_count


def parse_baseline(path: Path | None) -> set[str]:
    if path is None or not path.exists():
        return set()

    entries: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        entries.add(line)
    return entries


def write_report(path: Path, violations: list[Violation], files_scanned: int) -> None:
    by_pair = Counter((v.rule, v.source_layer, v.target_layer) for v in violations)
    examples: dict[tuple[str, str, str], list[Violation]] = defaultdict(list)
    for v in violations:
        key = (v.rule, v.source_layer, v.target_layer)
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
        lines.append("| Rule | Source Layer | Target Layer | Count |")
        lines.append("|---|---|---|---:|")
        for (rule, src, dst), count in by_pair.most_common():
            lines.append(f"| `{rule}` | `{src}` | `{dst}` | {count} |")

        lines.append("")
        lines.append("## Representative Violations")
        lines.append("")
        for key, _count in by_pair.most_common():
            rule, src, dst = key
            lines.append(f"### `{rule}`: `{src}` -> `{dst}`")
            for v in examples[key]:
                lines.append(f"- `{v.file}:{v.line}` includes `{v.include}`")
            lines.append("")

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def write_baseline(path: Path, violations: list[Violation]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "# Baseline keys for tools/lint/check_layer_references.py",
        "# Format: rule|file|line|source_layer|target_layer|include",
    ]
    for v in sorted(violations, key=lambda x: x.key()):
        lines.append(v.key())
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Check include-layer architecture references")
    parser.add_argument("--report", type=str, help="Optional path to write markdown report")
    parser.add_argument("--strict", action="store_true", help="Return non-zero when non-baselined violations are present")
    parser.add_argument("--baseline", type=str, help="Optional baseline file containing waived violations")
    parser.add_argument("--write-baseline", type=str, help="Write baseline entries for current violations to this path")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    violations, files_scanned = scan(repo_root)

    baseline_entries = parse_baseline((repo_root / args.baseline).resolve() if args.baseline else None)
    new_violations = [v for v in violations if v.key() not in baseline_entries]

    print(f"Scanned {files_scanned} code/header files")
    print(f"Found {len(violations)} total layer-reference violations")
    if baseline_entries:
        print(f"Baselined violations: {len(violations) - len(new_violations)}")
        print(f"New violations: {len(new_violations)}")

    if args.report:
        report_path = (repo_root / args.report).resolve()
        report_path.parent.mkdir(parents=True, exist_ok=True)
        write_report(report_path, new_violations, files_scanned)
        print(f"Report written to: {report_path}")

    if args.write_baseline:
        baseline_path = (repo_root / args.write_baseline).resolve()
        write_baseline(baseline_path, violations)
        print(f"Baseline written to: {baseline_path}")

    if args.strict and new_violations:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
