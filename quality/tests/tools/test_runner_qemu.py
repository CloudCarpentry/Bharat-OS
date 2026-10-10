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

@pytest.mark.parametrize(
    "arch, expected_exec",
    [
        ("x86_64", "qemu-system-x86_64"),
        ("arm64", "qemu-system-aarch64"),
        ("arm32", "qemu-system-arm"),
        ("riscv64", "qemu-system-riscv64"),
        ("riscv32", "qemu-system-riscv32"),
    ],
)
def test_build_qemu_command_architectures(arch, expected_exec):
    manifest = {"arch": arch, "run_config": {}, "artifacts": {}}
    cmd = build_qemu_command(manifest)
    assert cmd[0] == expected_exec


def test_build_qemu_command_hardware_config():
    manifest = {
        "arch": "x86_64",
        "run_config": {
            "machine": "q35",
            "cpu": "host",
            "memory": "1024M",
            "smp": 4,
        },
        "artifacts": {},
    }
    cmd = build_qemu_command(manifest)
    assert "-machine" in cmd
    assert cmd[cmd.index("-machine") + 1] == "q35"
    assert "-cpu" in cmd
    assert cmd[cmd.index("-cpu") + 1] == "host"
    assert "-m" in cmd
    assert cmd[cmd.index("-m") + 1] == "1024M"
    assert "-smp" in cmd
    assert cmd[cmd.index("-smp") + 1] == "4"


def test_build_qemu_command_artifacts_x86_64():
    manifest = {
        "arch": "x86_64",
        "run_config": {},
        "artifacts": {
            "boot_artifact": "kernel.elf",
            "init_module": "init.cpio",
            "root_module_name": "rootfs",
        },
    }
    cmd = build_qemu_command(manifest)
    assert "-kernel" in cmd
    assert cmd[cmd.index("-kernel") + 1] == "kernel.elf"
    assert "-initrd" in cmd
    assert cmd[cmd.index("-initrd") + 1] == "init.cpio rootfs"


def test_build_qemu_command_artifacts_arm():
    manifest = {
        "arch": "arm64",
        "run_config": {},
        "artifacts": {
            "boot_artifact": "kernel.bin",
            "init_module": "init.cpio",
            "dtb_artifact": "board.dtb",
        },
    }
    cmd = build_qemu_command(manifest)
    assert "-kernel" in cmd
    assert cmd[cmd.index("-kernel") + 1] == "kernel.bin"
    assert "-initrd" in cmd
    assert cmd[cmd.index("-initrd") + 1] == "init.cpio"
    assert "-dtb" in cmd
    assert cmd[cmd.index("-dtb") + 1] == "board.dtb"


@pytest.mark.parametrize(
    "nographic, display_override, is_windows, expected_display",
    [
        (True, None, False, "-nographic"),
        (False, None, False, "gtk"),
        (False, "headless", False, "-nographic"),
        (True, "gui", False, "gtk"),
        (True, None, True, "none"),
        (False, "headless", True, "none"),
    ],
)
def test_build_qemu_command_display_modes(monkeypatch, nographic, display_override, is_windows, expected_display):
    monkeypatch.setattr("sys.platform", "win32" if is_windows else "linux")
    manifest = {
        "arch": "x86_64",
        "run_config": {"nographic": nographic},
        "artifacts": {},
    }
    cmd = build_qemu_command(manifest, display_override=display_override)
    if expected_display == "-nographic":
        assert "-nographic" in cmd
    elif expected_display == "none":
        assert "-display" in cmd
        assert cmd[cmd.index("-display") + 1] == "none"
        assert "-serial" in cmd
        assert cmd[cmd.index("-serial") + 1] == "stdio"
    elif expected_display == "gtk":
        assert "-display" in cmd
        assert cmd[cmd.index("-display") + 1] == "gtk"
        assert "-device" in cmd
        assert "virtio-gpu-pci" in cmd
        assert "virtio-tablet-pci" in cmd


def test_build_qemu_command_extra_args():
    manifest = {
        "arch": "x86_64",
        "run_config": {
            "extra_args": ["-S", "-s", "-machine=ignored", "-machine", "also_ignored"]
        },
        "artifacts": {},
    }
    cmd = build_qemu_command(manifest)
    assert "-S" in cmd
    assert "-s" in cmd
    assert "-machine=ignored" not in cmd
    # We still have -machine from earlier default or config, but not the ones in extra_args


def test_build_qemu_command_missing_manifest_fields():
    manifest = {"arch": "x86_64"}
    cmd = build_qemu_command(manifest)
    assert cmd[0] == "qemu-system-x86_64"
    assert "-m" in cmd
    assert cmd[cmd.index("-m") + 1] == "512M"
    assert "-no-reboot" in cmd


def test_build_qemu_command_reboot_policy():
    manifest_stop = {
        "arch": "x86_64",
        "run_config": {"reboot_policy": "stop"},
    }
    cmd_stop = build_qemu_command(manifest_stop)
    assert "-no-reboot" in cmd_stop

    manifest_expect = {
        "arch": "x86_64",
        "run_config": {"reboot_policy": "expect"},
    }
    cmd_expect = build_qemu_command(manifest_expect)
    assert "-no-reboot" not in cmd_expect


def test_build_qemu_command_riscv32_bios(monkeypatch):
    monkeypatch.setattr("runner_qemu.Path.exists", lambda x: True)
    manifest = {
        "arch": "riscv32",
        "run_config": {},
    }
    cmd = build_qemu_command(manifest)
    assert "-bios" in cmd
    assert cmd[cmd.index("-bios") + 1] == "/usr/lib/riscv32-linux-gnu/opensbi/generic/fw_dynamic.bin"


def test_build_qemu_command_preserves_arch_boot_args():
    manifest = {
        "arch": "x86_64",
        "boot_contract": {
            "cmdline": "console=ttyS0 root=/dev/ram0"
        },
        "artifacts": {}
    }
    cmd = build_qemu_command(manifest)
    assert "-append" in cmd
    assert cmd[cmd.index("-append") + 1] == "console=ttyS0 root=/dev/ram0"
