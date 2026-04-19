from pathlib import Path

from tools.run.runner_qemu import run_qemu


def run_arm32(manifest_path: Path) -> int:
    return run_qemu(manifest_path)
