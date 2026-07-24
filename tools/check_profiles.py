#!/usr/bin/env python3
"""
Bharat-OS Profile Consistency Checker
Scans build_config.json for active profiles and ensures they are documented in README/docs.
"""

import json
import os
import re
import sys
from pathlib import Path

# Configuration
REPO_ROOT = Path(__file__).parent.parent
BUILD_CONFIG = REPO_ROOT / "build_config.json"
DOCS_DIRS = [
    REPO_ROOT / "README.md",
    REPO_ROOT / "docs/architecture",
]

# Exact normalized matching aliases
PROFILE_ALIASES = {
    "AUTOMOBILE": {"AUTOMOBILE", "AUTOMOTIVE", "AUTOMOTIVE_PROFILE"},
    "EV_AUTOMOBILE": {"EV_AUTOMOBILE", "EV_AUTOMOTIVE", "ELECTRIC_VEHICLE", "EV"},
    "MOBILE": {"MOBILE"},
    "IOT": {"IOT"},
    "LAPTOP": {"LAPTOP"},
    "MEDICAL": {"MEDICAL"},
}

def normalize_token(token):
    """Normalize a token for comparison: uppercase, replace - and spaces with _."""
    return token.upper().replace("-", "_").strip()

def get_build_profiles(config_path):
    """Extract unique profile names from build_config.json."""
    if not config_path.exists():
        print(f"Error: {config_path} not found.")
        return set()

    try:
        with open(config_path, "r") as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        print(f"Error parsing {config_path}: {e}")
        return set()

    profiles = set()
    builds = data.get("builds", {})
    for build_id, build_config in builds.items():
        profile = build_config.get("profile")
        if profile:
            profiles.add(profile)

    return profiles

def get_documented_tokens(paths):
    """Scan files and directories for uppercase tokens."""
    tokens = set()

    # Simple regex for uppercase tokens, allowing underscores
    token_re = re.compile(r'\b[A-Z][A-Z0-9_]+\b')

    for path in paths:
        if path.is_file():
            files = [path]
        elif path.is_dir():
            files = path.glob("*.md")
        else:
            continue

        for file in files:
            try:
                with open(file, "r", encoding="utf-8") as f:
                    content = f.read()
                    found = token_re.findall(content)
                    tokens.update(found)
            except Exception as e:
                print(f"Warning: Could not read {file}: {e}")

    return tokens

def main():
    print("Bharat-OS Profile Inventory Checker")
    print("-" * 40)

    build_profiles = get_build_profiles(BUILD_CONFIG)
    if not build_profiles:
        print("No build profiles found in build_config.json.")
        return 0

    print("Known build profiles:")
    for p in sorted(build_profiles):
        print(f"- {p}")

    doc_tokens = get_documented_tokens(DOCS_DIRS)

    # Check for undocumented profiles
    missing = []
    for profile in build_profiles:
        # Check for exact match
        if profile in doc_tokens:
            continue

        # Check aliases
        aliases = PROFILE_ALIASES.get(profile, set())
        if any(alias in doc_tokens for alias in aliases):
            continue

        missing.append(profile)

    print("\nDocumented profile tokens (subset):")
    # Only print tokens that resemble profiles for brevity
    relevant_tokens = sorted([t for t in doc_tokens if any(p in t or t in p for p in build_profiles) or t in ["RT", "SAFETY"]])
    for t in relevant_tokens[:15]:
        print(f"- {t}")
    if len(relevant_tokens) > 15:
        print(f"- ... ({len(relevant_tokens) - 15} more)")

    if missing:
        print("\nERR: The following build profiles are not mentioned in README or docs:")
        for m in missing:
            print(f"- {m}")
        print("\nPlease update README.md or docs/architecture/ to include these profiles.")
        return 1

    print("\nOK: All build_config profiles are mentioned in README/docs.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
