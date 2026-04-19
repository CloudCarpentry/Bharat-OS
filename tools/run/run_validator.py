import sys
from pathlib import Path


def validate_run_manifest(data: dict) -> None:
    if not data.get("runner_type"):
        print("Error: Run manifest missing 'runner_type'.")
        sys.exit(1)

    artifacts = data.get("artifacts", {})
    if not artifacts.get("boot_artifact"):
        print("Error: Run manifest missing 'boot_artifact'.")
        sys.exit(1)
