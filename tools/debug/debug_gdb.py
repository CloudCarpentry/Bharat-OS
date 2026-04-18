import json
import sys
from pathlib import Path


def load_debug_manifest(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"Debug manifest not found: {path}")
    with open(path, "r") as f:
        return json.load(f)


def build_gdb_command(data: dict) -> list[str]:
    debug_config = data.get("debug_config", {})
    artifacts = data.get("artifacts", {})

    symbols = artifacts.get("symbols")
    gdb_port = debug_config.get("gdb_port", 1234)

    cmd = ["gdb-multiarch"]

    if symbols:
        cmd.extend(["-ex", f"file {symbols}"])

    cmd.extend(["-ex", f"target remote localhost:{gdb_port}"])

    return cmd


def execute_debug(path: Path) -> int:
    manifest = load_debug_manifest(path)

    backend = manifest.get("debug_config", {}).get("backend")
    if backend != "gdb_remote":
        print(f"Error: Unsupported debug backend: {backend}")
        return 1

    cmd = build_gdb_command(manifest)
    print(f"[Debug] Run this command in a separate terminal:")
    print(" ".join(cmd))

    return 0
