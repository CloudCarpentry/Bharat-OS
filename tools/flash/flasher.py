import json
import sys
from pathlib import Path


def load_flash_manifest(path: Path) -> dict:
    if not path.exists():
        raise FileNotFoundError(f"Flash manifest not found: {path}")
    with open(path, "r") as f:
        return json.load(f)


def execute_flash(path: Path, dry_run: bool = False) -> int:
    manifest = load_flash_manifest(path)

    target_name = manifest.get("target_name", "unknown")
    flash_config = manifest.get("flash_config", {})
    backend = flash_config.get("backend")

    artifacts = manifest.get("artifacts", {})
    flash_artifact = artifacts.get("flash_artifact")

    print(f"\n[Flash] Starting flash procedure for {target_name}")
    print(f"[Flash] Backend: {backend}")
    print(f"[Flash] Artifact: {flash_artifact}")

    if not flash_artifact:
        print("Error: No flash artifact specified.")
        sys.exit(1)

    if backend == "openocd":
        from tools.flash.backends.openocd import execute_openocd
        return execute_openocd(manifest, dry_run)
    else:
        print(f"Error: Unsupported flash backend: {backend}")
        return 1
