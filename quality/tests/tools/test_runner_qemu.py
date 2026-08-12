import sys
from pathlib import Path


REPO_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(REPO_ROOT / "tools" / "run"))

from runner_qemu import build_qemu_command  # noqa: E402


def test_headless_override_keeps_gui_targets_display_capable():
    manifest = {
        "arch": "x86_64",
        "run_config": {"nographic": False},
        "artifacts": {},
    }

    command = build_qemu_command(manifest, display_override="headless")

    assert "-nographic" in command
    assert command.count("virtio-gpu-pci") == 1


def test_native_headless_target_does_not_gain_display_device():
    manifest = {
        "arch": "x86_64",
        "run_config": {"nographic": True},
        "artifacts": {},
    }

    command = build_qemu_command(manifest)

    assert "-nographic" in command
    assert "virtio-gpu-pci" not in command
