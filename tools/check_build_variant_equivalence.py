#!/usr/bin/env python3
"""Fail unless two resolved build configurations differ only in instrumentation."""

import argparse
import json
from pathlib import Path
from typing import Any

ALLOWED_INSTRUMENTATION = {
    "variant", "assertions", "symbols", "optimization", "tracing",
    "test_hooks", "poisoning", "invariant_checking",
}


def load(path: Path) -> dict[str, Any]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ValueError(f"cannot read {path}: {error}") from error
    if data.get("schema_version") != 1:
        raise ValueError(f"{path}: unsupported schema_version")
    if not isinstance(data.get("functional"), dict) or not isinstance(data.get("instrumentation"), dict):
        raise ValueError(f"{path}: functional and instrumentation objects are required")
    unknown = set(data["instrumentation"]) - ALLOWED_INSTRUMENTATION
    if unknown:
        raise ValueError(f"{path}: non-whitelisted instrumentation keys: {', '.join(sorted(unknown))}")
    return data


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("debug_config", type=Path)
    parser.add_argument("release_config", type=Path)
    args = parser.parse_args()
    try:
        left, right = load(args.debug_config), load(args.release_config)
    except ValueError as error:
        parser.error(str(error))
    if left["functional"] != right["functional"]:
        keys = sorted(set(left["functional"]) | set(right["functional"]))
        changed = [key for key in keys if left["functional"].get(key) != right["functional"].get(key)]
        print(json.dumps({"equivalent": False, "unexpected_functional_differences": changed}, sort_keys=True))
        return 1
    print(json.dumps({"equivalent": True, "allowed_difference_class": "instrumentation"}, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
