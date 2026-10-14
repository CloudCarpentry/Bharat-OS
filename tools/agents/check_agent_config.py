#!/usr/bin/env python3
"""
Bharat-OS Agent Configuration Linter
====================================
Validates all agent configurations, skills, and mappings:
1. Validates YAML frontmatter (name, description) in all .agents/skills/*/SKILL.md.
2. Checks for stale target references across all agent configuration files.
3. Checks that context_map.yaml references existing skills and source trees.
4. Checks that .claude/skills and .github/skills are in sync with .agents/skills.
"""

import re
import sys
from pathlib import Path
from typing import Dict, Any, List

try:
    import yaml
except ImportError:
    yaml = None

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

AGENTS_DIR = REPO_ROOT / ".agents"
SKILLS_DIR = AGENTS_DIR / "skills"
TARGETS_DIR = REPO_ROOT / "delivery" / "targets" / "qemu"
CONTEXT_MAP_PATH = REPO_ROOT / "tools" / "agents" / "context_map.yaml"

STALE_TARGET_PATTERNS = [
    re.compile(r"arm32_embedded_headless"),
    re.compile(r"powerpc_server_headless"),
    re.compile(r"powerpc_desktop"),
]


def check_skill_frontmatter() -> List[str]:
    errors = []
    if not SKILLS_DIR.exists():
        errors.append(f"Skills directory missing: {SKILLS_DIR}")
        return errors

    for skill_path in SKILLS_DIR.iterdir():
        if not skill_path.is_dir():
            continue
        skill_file = skill_path / "SKILL.md"
        if not skill_file.exists():
            errors.append(f"Missing SKILL.md in {skill_path.name}")
            continue

        content = skill_file.read_text(encoding="utf-8")
        if not content.startswith("---"):
            errors.append(f"{skill_file.name} in {skill_path.name} does not start with YAML frontmatter '---'")
            continue

        parts = content.split("---", 2)
        if len(parts) < 3:
            errors.append(f"{skill_file.name} in {skill_path.name} has malformed frontmatter delimiters")
            continue

        frontmatter_text = parts[1]
        try:
            data = yaml.safe_load(frontmatter_text) if yaml else {}
        except Exception as e:
            errors.append(f"Failed to parse frontmatter in {skill_path.name}: {e}")
            continue

        if not isinstance(data, dict):
            errors.append(f"Frontmatter in {skill_path.name} is not a valid YAML mapping")
            continue

        name = data.get("name")
        description = data.get("description")

        if not name:
            errors.append(f"Skill {skill_path.name} missing 'name' in frontmatter")
        elif name != skill_path.name:
            errors.append(f"Skill name mismatch: frontmatter '{name}' != directory '{skill_path.name}'")

        if not description:
            errors.append(f"Skill {skill_path.name} missing 'description' in frontmatter")

    return errors


def check_stale_targets() -> List[str]:
    errors = []
    files_to_check = [
        REPO_ROOT / "AGENTS.md",
        REPO_ROOT / "GEMINI.md",
        REPO_ROOT / ".clinerules",
        REPO_ROOT / ".cursorrules",
        REPO_ROOT / ".github" / "copilot-instructions.md",
    ]
    # Add all skill files
    if SKILLS_DIR.exists():
        files_to_check.extend(SKILLS_DIR.rglob("*.md"))

    for f in files_to_check:
        if not f.exists():
            continue
        try:
            content = f.read_text(encoding="utf-8")
            for pattern in STALE_TARGET_PATTERNS:
                match = pattern.search(content)
                if match:
                    rel = f.relative_to(REPO_ROOT)
                    errors.append(f"Stale target reference '{match.group(0)}' found in {rel}")
        except Exception as e:
            errors.append(f"Could not read {f}: {e}")

    return errors


def check_context_map() -> List[str]:
    errors = []
    if not CONTEXT_MAP_PATH.exists():
        errors.append(f"context_map.yaml missing at {CONTEXT_MAP_PATH}")
        return errors

    if not yaml:
        return errors

    try:
        data = yaml.safe_load(CONTEXT_MAP_PATH.read_text(encoding="utf-8")) or {}
    except Exception as e:
        errors.append(f"Error parsing context_map.yaml: {e}")
        return errors

    # Check skills referenced
    for area, info in data.items():
        skills = info.get("skills", [])
        for skill in skills:
            if not (SKILLS_DIR / skill / "SKILL.md").exists():
                errors.append(f"context_map area '{area}' references non-existent skill '{skill}'")

        sources = info.get("sources", [])
        for src in sources:
            src_path = REPO_ROOT / src
            if not src_path.exists():
                errors.append(f"context_map area '{area}' references non-existent source path '{src}'")

    return errors


def main():
    print("[Check] Validating Bharat-OS agent configuration and skills...")
    all_errors = []

    # 1. Frontmatter
    frontmatter_errors = check_skill_frontmatter()
    if frontmatter_errors:
        print("[FAIL] Skill frontmatter errors:")
        for err in frontmatter_errors:
            print(f"  - {err}")
        all_errors.extend(frontmatter_errors)
    else:
        print(f"[PASS] All {len(list(SKILLS_DIR.iterdir()))} skills have valid YAML frontmatter.")

    # 2. Stale target references
    stale_errors = check_stale_targets()
    if stale_errors:
        print("[FAIL] Stale target references found:")
        for err in stale_errors:
            print(f"  - {err}")
        all_errors.extend(stale_errors)
    else:
        print("[PASS] No stale target references detected.")

    # 3. Context map consistency
    context_errors = check_context_map()
    if context_errors:
        print("[FAIL] context_map.yaml errors:")
        for err in context_errors:
            print(f"  - {err}")
        all_errors.extend(context_errors)
    else:
        print("[PASS] context_map.yaml validated successfully.")

    # 4. Check sync
    from tools.agents.sync_skills import TARGET_DIRS, compare_directories
    for name, dst_dir in TARGET_DIRS:
        diffs = compare_directories(SKILLS_DIR, dst_dir)
        if diffs:
            msg = f"{name} skills out of sync with .agents/skills"
            print(f"[FAIL] {msg}")
            all_errors.append(msg)
        else:
            print(f"[PASS] {name} skills in sync.")

    if all_errors:
        print(f"\n[FAIL] Found {len(all_errors)} configuration issues.")
        sys.exit(1)
    else:
        print("\n[SUCCESS] Agent configuration and skill infrastructure fully verified!")


if __name__ == "__main__":
    main()
