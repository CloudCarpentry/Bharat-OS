#!/usr/bin/env python3
"""
Bharat-OS Skill Synchronizer
=============================
Syncs canonical skills from .agents/skills/ to:
- .claude/skills/ (Claude Code & Claude Dev)
- .github/skills/ (GitHub Copilot)

Ensures all agent environments share the exact same canonical skills.
"""

import argparse
import filecmp
import shutil
import sys
from pathlib import Path
from typing import List, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
CANONICAL_SKILLS_DIR = REPO_ROOT / ".agents" / "skills"
CLAUDE_SKILLS_DIR = REPO_ROOT / ".claude" / "skills"
GITHUB_SKILLS_DIR = REPO_ROOT / ".github" / "skills"

TARGET_DIRS = [
    ("Claude", CLAUDE_SKILLS_DIR),
    ("GitHub Copilot", GITHUB_SKILLS_DIR),
]


def compare_directories(src: Path, dst: Path) -> List[Tuple[str, str]]:
    """Compare contents of src and dst, returning list of (relative_path, diff_type)."""
    differences = []
    if not dst.exists():
        differences.append((str(dst.name), "DESTINATION_DIR_MISSING"))
        return differences

    # Check all files in src
    for src_file in src.rglob("*"):
        if src_file.is_file():
            rel_path = src_file.relative_to(src)
            dst_file = dst / rel_path
            if not dst_file.exists():
                differences.append((str(rel_path), "MISSING_IN_DST"))
            else:
                # Compare content normalizing line endings
                src_content = src_file.read_text(encoding="utf-8").replace("\r\n", "\n")
                dst_content = dst_file.read_text(encoding="utf-8").replace("\r\n", "\n")
                if src_content != dst_content:
                    differences.append((str(rel_path), "CONTENT_MISMATCH"))

    # Check for extraneous files in dst
    for dst_file in dst.rglob("*"):
        if dst_file.is_file():
            rel_path = dst_file.relative_to(dst)
            src_file = src / rel_path
            if not src_file.exists():
                differences.append((str(rel_path), "EXTRA_IN_DST"))

    return differences


def sync_to_destination(src: Path, dst: Path):
    """Mirror src directory to dst directory cleanly."""
    if dst.exists():
        shutil.rmtree(dst)
    dst.mkdir(parents=True, exist_ok=True)

    for item in src.iterdir():
        if item.is_dir():
            shutil.copytree(item, dst / item.name)
        elif item.is_file():
            shutil.copy2(item, dst / item.name)


def main():
    parser = argparse.ArgumentParser(description="Synchronize Bharat-OS skills across agent directories")
    parser.add_argument("--sync", action="store_true", help="Synchronize canonical skills to all platform target directories")
    parser.add_argument("--check", action="store_true", help="Check if platform skill directories are in sync with .agents/skills")

    args = parser.parse_args()

    if not args.sync and not args.check:
        args.sync = True  # Default action

    if not CANONICAL_SKILLS_DIR.exists():
        print(f"Error: Canonical skills directory not found: {CANONICAL_SKILLS_DIR}", file=sys.stderr)
        sys.exit(1)

    all_in_sync = True

    for name, dst_dir in TARGET_DIRS:
        if args.check:
            diffs = compare_directories(CANONICAL_SKILLS_DIR, dst_dir)
            if diffs:
                all_in_sync = False
                print(f"[FAIL] {name} skills ({dst_dir}) out of sync:")
                for rel, dtype in diffs:
                    print(f"  - {dtype}: {rel}")
            else:
                print(f"[PASS] {name} skills in sync with canonical .agents/skills/")
        elif args.sync:
            print(f"[Sync] Mirroring .agents/skills/ -> {name} ({dst_dir})...")
            sync_to_destination(CANONICAL_SKILLS_DIR, dst_dir)
            print(f"[PASS] Synced {name} skills.")

    if args.check and not all_in_sync:
        sys.exit(1)


if __name__ == "__main__":
    main()
