#!/usr/bin/env python3
"""Bharat-OS Doc Path Checker.

Validates that local file paths, directories, and backtick references in
documentation actually exist in the repository to prevent documentation-code drift.
"""

import os
import re
import sys
import glob
from pathlib import Path

# Match Markdown links like [label](path)
LINK_RE = re.compile(r'\[([^\]]+)\]\(([^)]+)\)')

# Match backtick references like `path`
BACKTICK_RE = re.compile(r'`([^`]+)`')

# Skip absolute web URLs
def is_web_url(url):
    return url.startswith("http://") or url.startswith("https://") or url.startswith("mailto:")

# Check if a string looks like a path reference (contains slashes or file extensions)
def looks_like_path(s):
    # If it contains standard path chars
    if "/" in s or "\\" in s:
        # Ignore things that look like commands/regex/code
        if any(c in s for c in [" ", "*", "$", "<", ">", "|", "!", "==", "=", "(", ")", "{", "}"]):
            return False
        # Ignore include headers (e.g. <string.h>)
        if s.startswith("<") or s.endswith(">"):
            return False
        return True
    # If it ends with typical documentation or source file extensions
    if any(s.endswith(ext) for ext in [".md", ".json", ".yaml", ".c", ".h", ".S", ".py", ".sh", ".ld", ".png", ".jpg"]):
        return True
    return False

def clean_path_reference(p, current_file_path, repo_root):
    # Strip anchors/hash links
    p = p.split("#")[0]
    if not p:
        return None

    # Handle relative paths vs absolute/root paths
    if p.startswith("/"):
        full_path = repo_root / p.lstrip("/")
    else:
        full_path = (Path(current_file_path).parent / p).resolve()

    return full_path

def scan_markdown_file(filepath, repo_root):
    violations = []
    try:
        content = Path(filepath).read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        print(f"Warning: Could not read {filepath}: {e}")
        return violations

    lines = content.splitlines()
    for line_idx, line in enumerate(lines, 1):
        # 1. Check Markdown links
        for match in LINK_RE.finditer(line):
            label, target = match.groups()
            if is_web_url(target):
                continue

            # Clean and resolve path
            resolved = clean_path_reference(target, filepath, repo_root)
            if resolved:
                if not resolved.exists():
                    violations.append({
                        "file": filepath,
                        "line": line_idx,
                        "ref_type": "Markdown Link",
                        "raw_ref": target,
                        "resolved_path": str(resolved.relative_to(repo_root) if resolved.is_relative_to(repo_root) else resolved)
                    })

        # 2. Check Backticks
        for match in BACKTICK_RE.finditer(line):
            target = match.group(1).strip()
            if looks_like_path(target):
                resolved = clean_path_reference(target, filepath, repo_root)
                # Also try resolving relative to repo root directly
                if resolved and not resolved.exists():
                    root_resolved = repo_root / target.replace("\\", "/").lstrip("/")
                    if not root_resolved.exists():
                        violations.append({
                            "file": filepath,
                            "line": line_idx,
                            "ref_type": "Backtick Path",
                            "raw_ref": target,
                            "resolved_path": str(root_resolved.relative_to(repo_root) if root_resolved.is_relative_to(repo_root) else root_resolved)
                        })

    return violations

def main():
    repo_root = Path(__file__).resolve().parents[2]

    # Locate all markdown files in repo (excluding build and node_modules)
    md_files = []
    for root, dirs, files in os.walk(repo_root):
        dirs[:] = [d for d in dirs if d not in {".git", "build", "node_modules"}]
        for file in files:
            if file.endswith(".md"):
                md_files.append(os.path.join(root, file))

    all_violations = []
    for f in md_files:
        rel_f = os.path.relpath(f, repo_root)
        violations = scan_markdown_file(f, repo_root)
        all_violations.extend(violations)

    print(f"--- Doc Path Checker Results ---")
    print(f"Scanned {len(md_files)} Markdown files.")
    print(f"Found {len(all_violations)} broken path references.\n")

    if all_violations:
        print("Broken References:")
        for v in all_violations:
            print(f"  [DRIFT] {os.path.relpath(v['file'], repo_root)}:{v['line']} ({v['ref_type']}): '{v['raw_ref']}' does not exist.")
        sys.exit(1)
    else:
        print("All local documentation paths are valid and up-to-date!")
        sys.exit(0)

if __name__ == "__main__":
    main()
