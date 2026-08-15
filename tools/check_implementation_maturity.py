#!/usr/bin/env python3
"""Fail-closed implementation-maturity gate for build targets."""

import argparse
import json
from pathlib import Path
import sys

MATURITIES = {"REAL", "PARTIAL", "TEST_ONLY", "STUB", "UNSUPPORTED"}
PRODUCTION_PROFILES = {"RELEASE", "HARDENED"}
PRODUCTION_FORBIDDEN = {"TEST_ONLY", "STUB"}


def load_manifest(path: Path) -> dict:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot load manifest: {exc}") from exc
    if data.get("schema_version") != 1 or not isinstance(data.get("implementations"), list):
        raise ValueError("schema_version must be 1 and implementations must be an array")
    seen = set()
    for index, entry in enumerate(data["implementations"]):
        if not isinstance(entry, dict):
            raise ValueError(f"implementations[{index}] must be an object")
        missing = {"id", "maturity", "sources", "evidence"} - set(entry)
        if missing:
            raise ValueError(f"implementations[{index}] missing: {', '.join(sorted(missing))}")
        if set(entry) - {"id", "maturity", "sources", "evidence"}:
            raise ValueError(f"implementations[{index}] contains unknown fields")
        if entry["id"] in seen:
            raise ValueError(f"duplicate implementation id: {entry['id']}")
        seen.add(entry["id"])
        if entry["maturity"] not in MATURITIES:
            raise ValueError(f"{entry['id']}: invalid maturity {entry['maturity']!r}")
        if not isinstance(entry["sources"], list) or not entry["sources"]:
            raise ValueError(f"{entry['id']}: sources must be a non-empty array")
        if not isinstance(entry["evidence"], str) or not entry["evidence"].strip():
            raise ValueError(f"{entry['id']}: evidence must be non-empty")
    return data


def check_manifest(data: dict, profile: str, allowed: set[str], repo_root: Path) -> list[str]:
    errors = []
    profile = profile.upper()
    if profile not in PRODUCTION_PROFILES | {"DEVELOPMENT", "DEBUG"}:
        return [f"unknown build profile {profile!r}"]
    for entry in data["implementations"]:
        maturity = entry["maturity"]
        for source in entry["sources"]:
            if not (repo_root / source).exists():
                errors.append(f"{entry['id']}: declared source does not exist: {source}")
        if profile in PRODUCTION_PROFILES and maturity in PRODUCTION_FORBIDDEN:
            errors.append(f"{entry['id']}: {maturity} is forbidden for {profile}")
        elif profile not in PRODUCTION_PROFILES and maturity in PRODUCTION_FORBIDDEN and maturity not in allowed:
            errors.append(f"{entry['id']}: {maturity} requires explicit target allowance for {profile}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=Path("interface/contracts/implementation_maturity.json"))
    parser.add_argument("--profile", required=True, choices=["RELEASE", "HARDENED", "DEVELOPMENT", "DEBUG"])
    parser.add_argument("--allow", action="append", default=[], choices=sorted(PRODUCTION_FORBIDDEN),
                        help="Explicit non-production allowance; repeat as needed")
    parser.add_argument("--repo-root", type=Path, default=Path.cwd())
    args = parser.parse_args()
    try:
        data = load_manifest(args.manifest)
        errors = check_manifest(data, args.profile, set(args.allow), args.repo_root)
    except ValueError as exc:
        print(f"ERROR: {exc}")
        return 2
    for error in errors:
        print(f"ERROR: {error}")
    if errors:
        print(f"FAILED: implementation maturity gate found {len(errors)} error(s)")
        return 1
    print(f"PASSED: {len(data['implementations'])} implementation declarations checked for {args.profile}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
