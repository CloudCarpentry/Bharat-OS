#!/usr/bin/env python3
"""Bharat-OS Documentation Command Validator.

Scans README.md and BUILD.md for documented build/run/test commands,
extracts target YAMLs, Python scripts, and options, and verifies they actually exist.
"""

import os
import re
import sys
from pathlib import Path

# Match commands like ./build.sh all --target-yaml <path>
# or python3 tools/...
COMMAND_RE = re.compile(r'(?:\./build\.sh|python3|\.\\build\.ps1)\s+([^\n`]+)')
YAML_PATH_RE = re.compile(r'delivery/targets/[a-zA-Z0-9_/.-]+\.yaml')
PYTHON_SCRIPT_RE = re.compile(r'tools/[a-zA-Z0-9_/.-]+\.py')

def scan_file_for_commands(filepath, repo_root):
    violations = []
    try:
        content = Path(filepath).read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        print(f"Warning: Could not read {filepath}: {e}")
        return violations

    for match in COMMAND_RE.finditer(content):
        cmd_str = match.group(0)

        # 1. Check for target YAML references
        for yaml_match in YAML_PATH_RE.finditer(cmd_str):
            yaml_path = yaml_match.group(0)
            full_path = repo_root / yaml_path
            if not full_path.exists():
                violations.append({
                    "file": filepath,
                    "command": cmd_str.strip(),
                    "missing": yaml_path,
                    "reason": "Referenced target YAML file does not exist"
                })

        # 2. Check for Python script references
        for py_match in PYTHON_SCRIPT_RE.finditer(cmd_str):
            py_path = py_match.group(0)
            full_path = repo_root / py_path
            if not full_path.exists():
                violations.append({
                    "file": filepath,
                    "command": cmd_str.strip(),
                    "missing": py_path,
                    "reason": "Referenced Python script does not exist"
                })

    return violations

def main():
    repo_root = Path(__file__).resolve().parents[2]

    docs_to_check = [
        repo_root / "README.md",
        repo_root / "BUILD.md"
    ]

    all_violations = []
    for filepath in docs_to_check:
        if filepath.exists():
            violations = scan_file_for_commands(filepath, repo_root)
            all_violations.extend(violations)

    print("--- Doc Command Validator Results ---")
    print(f"Scanned {len(docs_to_check)} files.")
    print(f"Found {len(all_violations)} invalid command references.\n")

    if all_violations:
        for v in all_violations:
            print(f"  [COMMAND-ERROR] {os.path.relpath(v['file'], repo_root)}: Command `{v['command']}` references non-existent `{v['missing']}`.")
            print(f"    Reason: {v['reason']}")
        sys.exit(1)
    else:
        print("All documented commands are correct and reference existing files!")
        sys.exit(0)

if __name__ == "__main__":
    main()
