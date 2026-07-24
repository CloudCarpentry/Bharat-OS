#!/usr/bin/env python3
"""Bharat-OS Front-Matter Injected/Formatter.

Recursively scans all Markdown files under the `docs/` folder and ensures
they have a standard, complete YAML front matter containing:
- title
- status
- owner
- last_updated
- tags
- see_also

Preserves any other existing fields while keeping the formatting consistent.
"""

import os
import re
import sys
import yaml
from pathlib import Path

def parse_markdown_file(content):
    # Normalize CRLF to LF
    content = content.replace('\r\n', '\n')

    # Split by lines and check if the first line is ---
    lines = content.split('\n')
    if len(lines) > 1 and lines[0].strip() == "---":
        closing_idx = -1
        for i in range(1, len(lines)):
            if lines[i].strip() == "---":
                closing_idx = i
                break
        if closing_idx != -1:
            front_matter_raw = "\n".join(lines[1:closing_idx])
            markdown_body = "\n".join(lines[closing_idx+1:])

            # Try parsing with PyYAML first
            try:
                data = yaml.safe_load(front_matter_raw)
                if isinstance(data, dict):
                    return data, markdown_body
            except Exception as e:
                print(f"Warning: Failed to parse YAML front matter as dictionary: {e}")

            # Fallback line-by-line parsing if YAML safe_load failed
            data = {}
            for line in front_matter_raw.split('\n'):
                if ":" in line:
                    parts = line.split(":", 1)
                    k = parts[0].strip()
                    v = parts[1].strip()
                    # Strip surrounding quotes/backticks
                    if (v.startswith('"') and v.endswith('"')) or (v.startswith("'") and v.endswith("'")):
                        v = v[1:-1].strip()
                    elif v.startswith("`") and v.endswith("`"):
                        v = v[1:-1].strip()
                    data[k] = v
            return data, markdown_body

    return {}, content

def serialize_yaml(data):
    lines = []
    lines.append("---")

    ordered_keys = ["title", "status", "owner", "last_updated", "tags", "see_also"]
    all_keys = list(data.keys())

    # Filter ordered_keys to only those present in data
    keys_to_write = [k for k in ordered_keys if k in data]
    # Add any other keys not in ordered_keys
    for k in all_keys:
        if k not in keys_to_write:
            keys_to_write.append(k)

    for k in keys_to_write:
        v = data[k]
        if isinstance(v, list):
            lines.append(f"{k}:")
            for item in v:
                lines.append(f"  - {item}")
        else:
            val_str = str(v).strip()
            # Avoid wrapping if already nicely quoted
            if ":" in val_str or "#" in val_str or "`" in val_str or val_str.startswith("-") or val_str.startswith("[") or val_str.startswith("{"):
                if not ((val_str.startswith('"') and val_str.endswith('"')) or (val_str.startswith("'") and val_str.endswith("'"))):
                    val_str = f'"{val_str}"'
            lines.append(f"{k}: {val_str}")

    lines.append("---")
    return "\n".join(lines)

def process_file(file_path, docs_root):
    content = file_path.read_text(encoding="utf-8", errors="ignore")

    existing_fm, markdown_body = parse_markdown_file(content)

    # 1. Determine Title
    title = existing_fm.get("title")
    if not title:
        # Find first markdown header
        header_match = re.search(r'^\s*#\s+(.+)$', markdown_body, re.MULTILINE)
        if header_match:
            title = header_match.group(1).strip()
        else:
            # Derive from filename
            name_without_ext = file_path.stem
            title = name_without_ext.replace("-", " ").replace("_", " ").title()

    status = existing_fm.get("status", "Draft")
    owner = existing_fm.get("owner", "Documentation Working Group")
    last_updated = existing_fm.get("last_updated", "2026-04-25")

    # 2. Inferred tags
    tags = existing_fm.get("tags")
    if not tags:
        tags = ["docs"]
        try:
            rel_parent = file_path.parent.relative_to(docs_root)
            for part in rel_parent.parts:
                if part and part != ".":
                    tags.append(part)
        except Exception:
            pass
    elif isinstance(tags, str):
        tags = [tags]
    elif isinstance(tags, list):
        # Clean any non-string entries
        tags = [str(t) for t in tags]

    # 3. Inferred see_also
    see_also = existing_fm.get("see_also")
    if not see_also:
        if file_path.name.lower() == "readme.md":
            if file_path.parent == docs_root:
                see_also = ["architecture/README.md", "adr/README.md"]
            else:
                see_also = ["../README.md"]
        else:
            see_also = ["README.md"]
    elif isinstance(see_also, str):
        see_also = [see_also]
    elif isinstance(see_also, list):
        see_also = [str(s) for s in see_also]

    # Construct complete/updated front matter
    new_fm = existing_fm.copy()
    new_fm["title"] = title
    new_fm["status"] = status
    new_fm["owner"] = owner
    new_fm["last_updated"] = last_updated
    new_fm["tags"] = tags
    new_fm["see_also"] = see_also

    serialized_fm = serialize_yaml(new_fm)

    # Clean up the markdown body leading newlines
    markdown_body = markdown_body.lstrip("\r\n ")

    new_content = f"{serialized_fm}\n{markdown_body}"

    if content != new_content:
        file_path.write_text(new_content, encoding="utf-8")
        return True
    return False

def main():
    repo_root = Path(__file__).resolve().parents[2]
    docs_root = repo_root / "docs"

    if not docs_root.exists():
        print(f"Error: docs folder '{docs_root}' not found.")
        sys.exit(1)

    count = 0
    updated = 0
    for root, _, files in os.walk(docs_root):
        for file in files:
            if file.endswith(".md"):
                count += 1
                file_path = Path(root) / file
                try:
                    if process_file(file_path, docs_root):
                        updated += 1
                except Exception as e:
                    print(f"Error processing {file_path}: {e}")

    print(f"Scan complete. Inspected {count} Markdown files. Updated {updated} files.")

if __name__ == "__main__":
    main()
