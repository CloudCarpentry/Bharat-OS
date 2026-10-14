#!/usr/bin/env python3
"""
Bharat-OS Context Pack Generator for Autonomous Agents
======================================================
Produces a compact, structured context pack for a subsystem or set of changed files.
Cross-references tools/agents/context_map.yaml and interface/contracts/implementation_maturity.json.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, Any, List, Optional

try:
    import yaml
except ImportError:
    yaml = None

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
CONTEXT_MAP_PATH = REPO_ROOT / "tools" / "agents" / "context_map.yaml"
MATURITY_PATH = REPO_ROOT / "interface" / "contracts" / "implementation_maturity.json"


def load_context_map() -> Dict[str, Any]:
    if not CONTEXT_MAP_PATH.exists():
        return {}
    if yaml is None:
        # Fallback if yaml is somehow unavailable
        return {}
    with open(CONTEXT_MAP_PATH, "r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


def load_maturity_data() -> Dict[str, Any]:
    if not MATURITY_PATH.exists():
        return {}
    try:
        with open(MATURITY_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return {}


def find_maturity_for_area(area_key: str, area_info: Dict[str, Any], maturity_data: Dict[str, Any]) -> Dict[str, Any]:
    implementations = maturity_data.get("implementations", [])
    area_norm = area_key.replace("-", "_").lower()
    
    for impl in implementations:
        impl_id = impl.get("id", "").lower()
        if area_norm in impl_id or impl_id.startswith(area_norm):
            return impl
        for src in impl.get("sources", []):
            for area_src in area_info.get("sources", []):
                if src in area_src or area_src in src:
                    return impl
    return {
        "maturity": "VERIFIED_OR_CORE",
        "evidence": "Active core subsystem without separate partial maturity flag."
    }


def match_area_by_files(changed_files: List[str], context_map: Dict[str, Any]) -> List[str]:
    matched_areas = set()
    for file_str in changed_files:
        fpath = file_str.replace("\\", "/").lower()
        for area_key, area_info in context_map.items():
            for src in area_info.get("sources", []):
                src_norm = src.replace("\\", "/").lower()
                if src_norm in fpath or fpath in src_norm:
                    matched_areas.add(area_key)
    return sorted(list(matched_areas))


def generate_compact_context(area_key: str, area_info: Dict[str, Any], maturity_info: Dict[str, Any]) -> str:
    lines = [
        f"=== Bharat-OS Context Pack: {area_info.get('name', area_key)} ===",
        f"subsystem: {area_key}",
        f"layer: {area_info.get('layer', 'unknown')} | owns_policy: {area_info.get('policy', False)}",
        f"maturity: {maturity_info.get('maturity', 'UNKNOWN')}",
        f"evidence: {maturity_info.get('evidence', 'None')}",
        f"protection_models: {', '.join(area_info.get('protection_models', ['ALL']))}",
        "",
        "canonical_skills_to_load:",
    ]
    for skill in area_info.get("skills", []):
        lines.append(f"  - {skill}")
    
    lines.append("")
    lines.append("primary_sources:")
    for src in area_info.get("sources", []):
        lines.append(f"  - {src}")
        
    lines.append("")
    lines.append("authoritative_contracts:")
    for contract in area_info.get("contracts", []):
        lines.append(f"  - {contract}")

    lines.append("")
    lines.append("focused_tests:")
    for test in area_info.get("tests", []):
        lines.append(f"  - {test}")

    lines.append("")
    lines.append("runtime_markers:")
    for marker in area_info.get("runtime_markers", []):
        lines.append(f"  - {marker}")

    lines.append("")
    lines.append("mandatory_qualification:")
    lines.append("  - python3 tools/agents/verify.py --changed-files <...>")
    lines.append("  - python3 tools/run_qemu_matrix.py --headless --smoke --all-arch")
    lines.append("")
    lines.append("forbidden_shortcuts:")
    lines.append("  - No global mutable state (use core-local ownership)")
    lines.append("  - No raw pointers across IPC/uRPC/wire boundaries")
    lines.append("  - No spin-count waits (deadlines must use monotonic bh_ktime_now())")
    lines.append("  - Never hand-edit generated headers or syscall tables")
    lines.append("================================================================")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Generate compact agent context pack")
    parser.add_argument("--area", type=str, help="Subsystem area key (e.g. vm-manager, scheduler)")
    parser.add_argument("--changed-files", nargs="*", help="List of changed file paths")
    parser.add_argument("--list-areas", action="store_true", help="List all available area keys")
    parser.add_argument("--format", choices=["compact", "json"], default="compact", help="Output format")

    args = parser.parse_args()
    context_map = load_context_map()
    maturity_data = load_maturity_data()

    if args.list_areas:
        print("Available Subsystem Areas in context_map.yaml:")
        for k, v in context_map.items():
            print(f"  - {k:20s}: {v.get('name', '')} ({v.get('layer', '')})")
        return

    target_areas = []
    if args.area:
        target_areas.append(args.area)
    elif args.changed_files:
        target_areas = match_area_by_files(args.changed_files, context_map)
        if not target_areas:
            print("[ContextPack] No specific area matched changed files. Defaulting to general kernel architecture.")
            target_areas = ["scheduler"]
    else:
        parser.print_help()
        sys.exit(1)

    for area_key in target_areas:
        if area_key not in context_map:
            print(f"Error: Unknown area key '{area_key}'. Run with --list-areas to see choices.", file=sys.stderr)
            sys.exit(1)

        area_info = context_map[area_key]
        maturity_info = find_maturity_for_area(area_key, area_info, maturity_data)

        if args.format == "compact":
            print(generate_compact_context(area_key, area_info, maturity_info))
        elif args.format == "json":
            payload = {
                "area": area_key,
                "info": area_info,
                "maturity": maturity_info,
                "gates": [
                    "python3 tools/agents/verify.py",
                    "python3 tools/run_qemu_matrix.py --headless --smoke --all-arch"
                ]
            }
            print(json.dumps(payload, indent=2))


if __name__ == "__main__":
    main()
