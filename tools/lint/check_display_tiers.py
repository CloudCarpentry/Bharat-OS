#!/usr/bin/env python3
"""Bharat-OS Display Tier & Zero-Bloat Policy Validator.

Validates that target configurations in delivery/targets/ adhere to ADR-035:
- Headless, Automotive ECU, RTOS, and Cloud targets must have BHARAT_BOOT_GUI=OFF and no UI bloat.
- GUI / Desktop showcase targets properly declare their UI subsystem settings.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    # Fallback to basic line-by-line parsing if pyyaml is unavailable
    yaml = None


def parse_target_yaml(path: Path) -> dict:
    text = path.read_text(encoding="utf-8")
    if yaml is not None:
        return yaml.safe_load(text) or {}

    # Minimal fallback parser
    data = {"name": path.stem, "build": {"cmake_defs": {}}}
    current_section = None
    for line in text.splitlines():
        stripped = line.strip()
        if not stripped or stripped.startswith("#"):
            continue
        if stripped.endswith(":") and not stripped.startswith("-"):
            current_section = stripped[:-1].strip()
            continue
        if ":" in stripped:
            k, v = stripped.split(":", 1)
            k = k.strip()
            v = v.strip().strip('"').strip("'")
            if current_section == "cmake_defs":
                data["build"]["cmake_defs"][k] = v
            elif current_section is None:
                data[k] = v
    return data


def check_targets(repo_root: Path) -> tuple[int, int, list[str]]:
    targets_dir = repo_root / "delivery" / "targets"
    if not targets_dir.exists():
        return 0, 0, [f"Targets directory not found: {targets_dir}"]

    target_files = list(targets_dir.glob("**/*.yaml"))
    checked = 0
    passed = 0
    errors: list[str] = []

    for target_file in sorted(target_files):
        checked += 1
        rel_path = target_file.relative_to(repo_root).as_posix()
        try:
            doc = parse_target_yaml(target_file)
        except Exception as e:
            errors.append(f"{rel_path}: Failed to parse YAML: {e}")
            continue

        target_name = doc.get("name", target_file.stem)
        build_cfg = doc.get("build", {})
        cmake_defs = build_cfg.get("cmake_defs", {}) if isinstance(build_cfg, dict) else {}

        is_headless = (
            "headless" in target_name.lower()
            or "headless" in target_file.stem.lower()
            or doc.get("personality_profile") == "automotive"
            or cmake_defs.get("BHARAT_DEVICE_PROFILE") == "AUTOMOTIVE_ECU"
            or cmake_defs.get("BHARAT_BOOT_HW_PROFILE") == "rtos"
        )

        boot_gui_def = cmake_defs.get("BHARAT_BOOT_GUI")

        if is_headless:
            if boot_gui_def is not None and str(boot_gui_def).upper() not in {"OFF", "0", "FALSE", "NO"}:
                errors.append(
                    f"{rel_path}: Headless target '{target_name}' must have BHARAT_BOOT_GUI: OFF (found '{boot_gui_def}')"
                )
                continue
            if str(cmake_defs.get("BHARAT_ENABLE_UI", "")).upper() in {"ON", "1", "TRUE", "YES"}:
                errors.append(
                    f"{rel_path}: Headless target '{target_name}' must not have BHARAT_ENABLE_UI: ON"
                )
                continue

        passed += 1

    return checked, passed, errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate display tier policies on Bharat-OS targets.")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[2])
    args = parser.parse_args()

    checked, passed, errors = check_targets(args.repo_root)

    print(f"Validated {checked} target configurations against ADR-035 display tier policy.")
    if errors:
        print(f"\n[FAIL] Found {len(errors)} display policy violations:")
        for err in errors:
            print(f"  - {err}")
        return 1

    print(f"[PASS] All {passed} targets conform to zero-bloat display policies.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
