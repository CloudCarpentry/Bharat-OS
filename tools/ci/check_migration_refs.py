#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
LEGACY_PATTERNS = ("tools/targets/qemu/",)
ALLOWED_FILES = {
    "BUILD.md",
    "project-structure-refactor-plan.md",
    "tools/build/path_aliases.py",
}


def changed_files(base_ref: str) -> list[str]:
    cmd = ["git", "diff", "--name-only", f"{base_ref}...HEAD"]
    result = subprocess.run(cmd, cwd=REPO_ROOT, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "unable to list changed files")
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def check_file(path: Path) -> list[str]:
    rel = path.relative_to(REPO_ROOT).as_posix()
    if rel in ALLOWED_FILES:
        return []
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        return []

    violations: list[str] = []
    for pattern in LEGACY_PATTERNS:
        if pattern in text:
            violations.append(pattern)
    return violations


def main() -> int:
    parser = argparse.ArgumentParser(description="Guard against new legacy migration references.")
    parser.add_argument("--base-ref", default="origin/main", help="Git ref used for changed-file diff.")
    parser.add_argument("--strict", action="store_true", help="Fail on violations. Default warns only.")
    args = parser.parse_args()

    try:
        files = changed_files(args.base_ref)
    except RuntimeError as exc:
        print(f"[migration-warning] {exc}")
        return 0

    found = False
    for rel in files:
        path = REPO_ROOT / rel
        if not path.is_file():
            continue
        violations = check_file(path)
        if violations:
            found = True
            for violation in violations:
                print(f"[migration-warning] {rel}: found legacy reference '{violation}'")

    if found and args.strict:
        print("[migration-error] Legacy references introduced in changed files.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
