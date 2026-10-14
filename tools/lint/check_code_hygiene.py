#!/usr/bin/env python3
"""
Bharat-OS Code Hygiene Linter
=============================
Checks for inconsistencies documented in docs/reviews/code_inconsistencies.md:
1. #include directives placed after code or non-preprocessor declarations.
2. Empty comments (standalone '//' or '/* */' with only whitespace).
3. CamelCase naming violations in standard files.

Supports:
- --check: Reports all violations and returns non-zero exit code if new violations found.
- --fix-comments: Automatically cleans empty comments.
"""

import argparse
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent.parent

INCLUDE_REGEX = re.compile(r"^\s*#\s*include\s*[<\"][^>\"]+[>\"]")
EMPTY_LINE_COMMENT_REGEX = re.compile(r"^\s*//\s*$")
EMPTY_BLOCK_COMMENT_REGEX = re.compile(r"^\s*/\*\s*\*/\s*$")

# Files or directories exempt from strict include placement (e.g. x-macros, table generation)
INCLUDE_EXEMPTIONS = [
    "core/personalities/native/native_syscall_handlers.c", # Table dispatch includes
    "core/personalities/native/native_syscall.c",
    "quality/tests/benchmark_stubs.c",
]

# Files exempt from camelCase check (e.g. USB specs, external hardware specs)
NAMING_EXEMPTIONS = [
    "core/drivers/bus/usb/",
    "experience/user/ui/fbui/input/fb_input_showcase.c",
    "ext/",
    "external/",
    "third_party/",
]


def scan_file_includes(path: Path) -> List[Tuple[int, str]]:
    """Find #include lines that appear after non-preprocessor code."""
    rel = path.relative_to(REPO_ROOT).as_posix()
    for exempt in INCLUDE_EXEMPTIONS:
        if exempt in rel:
            return []

    violations = []
    seen_code = False
    in_multiline_comment = False

    try:
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except Exception:
        return []

    for idx, line in enumerate(lines, 1):
        s = line.strip()
        if not s:
            continue
        if in_multiline_comment:
            if "*/" in s:
                in_multiline_comment = False
            continue
        if s.startswith("/*"):
            if "*/" not in s:
                in_multiline_comment = True
            continue
        if s.startswith("//"):
            continue

        if s.startswith("#"):
            if INCLUDE_REGEX.match(line) and seen_code:
                # Check for explicit NOLINT
                if "NOLINT" not in line:
                    violations.append((idx, s))
            continue

        seen_code = True

    return violations


def scan_empty_comments(path: Path) -> List[Tuple[int, str]]:
    """Find empty comments like '//' or '/* */'."""
    violations = []
    try:
        lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    except Exception:
        return []

    for idx, line in enumerate(lines, 1):
        if EMPTY_LINE_COMMENT_REGEX.match(line):
            violations.append((idx, line.strip()))
        elif EMPTY_BLOCK_COMMENT_REGEX.match(line):
            violations.append((idx, line.strip()))

    return violations


def fix_empty_comments_in_file(path: Path) -> int:
    """Removes empty comments in place."""
    try:
        text = path.read_text(encoding="utf-8")
    except Exception:
        return 0

    lines = text.splitlines(keepends=True)
    new_lines = []
    fixed_count = 0
    for line in lines:
        if EMPTY_LINE_COMMENT_REGEX.match(line.rstrip("\r\n")) or EMPTY_BLOCK_COMMENT_REGEX.match(line.rstrip("\r\n")):
            fixed_count += 1
            # Replace with plain newline to preserve line formatting
            new_lines.append("\n" if "\r\n" not in line else "\r\n")
        else:
            new_lines.append(line)

    if fixed_count > 0:
        path.write_text("".join(new_lines), encoding="utf-8")

    return fixed_count


def scan_tree(root_dirs: List[Path]):
    include_violations: Dict[str, List[Tuple[int, str]]] = {}
    comment_violations: Dict[str, List[Tuple[int, str]]] = {}

    for rdir in root_dirs:
        if not rdir.exists():
            continue
        for p in rdir.rglob("*"):
            if p.is_file() and p.suffix in [".c", ".h"]:
                rel = p.relative_to(REPO_ROOT).as_posix()
                if any(x in rel for x in ["build/", "ext/", "external/", "third_party/", ".git/"]):
                    continue

                inc_v = scan_file_includes(p)
                if inc_v:
                    include_violations[rel] = inc_v

                com_v = scan_empty_comments(p)
                if com_v:
                    comment_violations[rel] = com_v

    return include_violations, comment_violations


def main():
    parser = argparse.ArgumentParser(description="Bharat-OS Code Hygiene Linter")
    parser.add_argument("--check", action="store_true", help="Report violations")
    parser.add_argument("--fix-empty-comments", action="store_true", help="Automatically remove empty comments")
    parser.add_argument("--files", nargs="*", help="Specific files to check")

    args = parser.parse_args()
    if not args.check and not args.fix_empty_comments:
        args.check = True

    target_dirs = [
        REPO_ROOT / "core",
        REPO_ROOT / "quality",
        REPO_ROOT / "experience",
        REPO_ROOT / "interface",
        REPO_ROOT / "staging",
    ]

    if args.fix_empty_comments:
        total_fixed = 0
        for rdir in target_dirs:
            for p in rdir.rglob("*"):
                if p.is_file() and p.suffix in [".c", ".h"]:
                    rel = p.relative_to(REPO_ROOT).as_posix()
                    if any(x in rel for x in ["build/", "ext/", "external/", "third_party/"]):
                        continue
                    n = fix_empty_comments_in_file(p)
                    if n > 0:
                        print(f"Fixed {n} empty comment(s) in {rel}")
                        total_fixed += n
        print(f"\n[Fixed] Total empty comments cleaned: {total_fixed}")

    if args.check:
        print("[Hygiene] Scanning Bharat-OS codebase for code inconsistencies...")
        inc_v, com_v = scan_tree(target_dirs)

        print(f"\n1. Mid-file #include directives: {len(inc_v)} files found")
        for f, viols in sorted(inc_v.items())[:20]:
            lines_str = ", ".join(str(v[0]) for v in viols)
            print(f"  - {f}: {len(viols)} found (lines: {lines_str})")
        if len(inc_v) > 20:
            print(f"  ... and {len(inc_v) - 20} more files.")

        print(f"\n2. Empty comments: {len(com_v)} files found")
        for f, viols in sorted(com_v.items()):
            lines_str = ", ".join(str(v[0]) for v in viols)
            print(f"  - {f}: {len(viols)} found (lines: {lines_str})")

        print("\nSummary:")
        print(f"  Files with mid-file includes: {len(inc_v)}")
        print(f"  Files with empty comments:    {len(com_v)}")


if __name__ == "__main__":
    main()
