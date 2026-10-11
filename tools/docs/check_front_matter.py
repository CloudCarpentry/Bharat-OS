#!/usr/bin/env python3
"""Bharat-OS Doc Front-Matter Checker.

Validates that all Markdown files in the `docs/` folder have a complete and valid
YAML front-matter header containing the standard metadata keys.
"""

import os
import re
import sys
import yaml
from pathlib import Path

REQUIRED_KEYS = ["title", "status", "owner", "last_updated", "tags", "see_also"]

def validate_front_matter(file_path):
    try:
        content = file_path.read_text(encoding="utf-8", errors="ignore")
    except Exception as e:
        return [f"Could not read file: {e}"]

    # Normalize CRLF to LF
    content = content.replace('\r\n', '\n')

    # Split by lines and check if the first line is ---
    lines = content.split('\n')
    if not (len(lines) > 1 and lines[0].strip() == "---"):
        return ["Missing front-matter delimiters ('---') at start of file."]

    closing_idx = -1
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            closing_idx = i
            break
    if closing_idx == -1:
        return ["Missing closing front-matter delimiter ('---')."]

    front_matter_raw = "\n".join(lines[1:closing_idx])
    markdown_body = "\n".join(lines[closing_idx+1:])

    try:
        data = yaml.safe_load(front_matter_raw)
    except Exception as e:
        return [f"Failed to parse YAML front matter: {e}"]

    if not isinstance(data, dict):
        return ["Front matter is not a valid YAML dictionary."]

    violations = []

    # Check if there's any duplicate '---' block in the body
    body_lines = [l.strip() for l in markdown_body.split('\n') if l.strip()]
    for idx, line in enumerate(body_lines):
        if line == "---" and idx + 1 < len(body_lines):
            next_line = body_lines[idx + 1]
            if any(next_line.startswith(k + ":") for k in REQUIRED_KEYS):
                violations.append("Detected potential duplicate front-matter block in document body.")
                break

    # Check for required keys
    for key in REQUIRED_KEYS:
        if key not in data:
            violations.append(f"Missing required metadata key: '{key}'")
            continue

        # Check types and content
        val = data[key]
        # Treat empty list as not purely "empty" for empty check
        if val is None or (isinstance(val, str) and val.strip() == "") or (not isinstance(val, list) and str(val).strip() == ""):
            violations.append(f"Metadata key '{key}' is empty.")
            continue

        if key == "last_updated":
            if not re.match(r"^\d{4}-\d{2}-\d{2}$", str(val).strip()):
                violations.append(f"Metadata key '{key}' must be in YYYY-MM-DD format (got '{val}').")
        elif key == "tags":
            if not isinstance(val, list):
                violations.append(f"Metadata key '{key}' must be a list (got {type(val).__name__}).")
            elif len(val) == 0:
                violations.append(f"Metadata key '{key}' cannot be an empty list.")
            else:
                for idx, item in enumerate(val):
                    if not isinstance(item, str):
                        violations.append(f"Metadata key '{key}' elements must be strings (element {idx} is {type(item).__name__}).")
                    elif str(item).strip() == "":
                        violations.append(f"Metadata key '{key}' elements cannot be blank strings (element {idx} is blank).")
        elif key == "see_also":
            if not isinstance(val, list):
                violations.append(f"Metadata key '{key}' must be a list (got {type(val).__name__}).")
            else:
                for idx, item in enumerate(val):
                    if not isinstance(item, str):
                        violations.append(f"Metadata key '{key}' elements must be strings (element {idx} is {type(item).__name__}).")
                    elif str(item).strip() == "":
                        violations.append(f"Metadata key '{key}' elements cannot be blank strings (element {idx} is blank).")

    return violations

def main():
    repo_root = Path(__file__).resolve().parents[2]
    docs_root = repo_root / "docs"

    if not docs_root.exists():
        print(f"Error: docs folder '{docs_root}' not found.")
        sys.exit(1)

    md_files = list(docs_root.rglob("*.md"))

    all_violations = {}
    for file_path in md_files:
        violations = validate_front_matter(file_path)
        if violations:
            all_violations[file_path.relative_to(repo_root)] = violations

    print(f"--- Doc Front-Matter Checker Results ---")
    print(f"Scanned {len(md_files)} Markdown files.")
    print(f"Found {len(all_violations)} files with front-matter violations.\n")

    if all_violations:
        for path, violations in all_violations.items():
            print(f"  [METADATA-ERROR] {path}:")
            for v in violations:
                print(f"    - {v}")
        sys.exit(1)
    else:
        print("All documentation files have valid and complete front-matter headers!")
        sys.exit(0)

if __name__ == "__main__":
    main()
