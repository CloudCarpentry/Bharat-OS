import sys
import json
import pytest
from pathlib import Path


REPO_ROOT = Path(__file__).parents[3]
sys.path.insert(0, str(REPO_ROOT / "tools" / "run"))

from runner_qemu import build_qemu_command  # noqa: E402
import runner_qemu  # noqa: E402


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


@pytest.mark.parametrize("mode", ["smoke", "interactive"])
def test_kernel_entry_does_not_hide_bootstrap_failure(tmp_path, monkeypatch, mode):
    manifest = tmp_path / "run-manifest.json"
    manifest.write_text(json.dumps({
        "target_name": "unlisted_gui", "arch": "x86_64", "run_config": {},
    }))
    guest = tmp_path / "guest.py"
    guest.write_text(
        'import time\n'
        'print("BOOT: kernel_main reached", flush=True)\n'
        'print("BOOT_FAIL: INIT_BOOTSTRAP", flush=True)\n'
        'time.sleep(30)\n'
    )
    monkeypatch.setattr(runner_qemu, "__file__", str(tmp_path / "tools/run/runner_qemu.py"))
    monkeypatch.setattr(runner_qemu, "build_qemu_command", lambda *args: [sys.executable, str(guest)])
    assert runner_qemu.run_qemu(manifest, mode_override=mode) == 1
    evidence = json.loads((tmp_path / "build/evidence/unlisted_gui_evidence.json").read_text())
    assert evidence["runtime_result"] == "FAIL"
    assert "BOOT_FAIL:" in evidence["forbidden_markers"]
