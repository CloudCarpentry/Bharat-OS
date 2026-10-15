import json
import sys
import pytest
from pathlib import Path

REPO_ROOT = Path(__file__).parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.runners.runner_renode import (
    find_renode_executable,
    build_resc_script,
    run_renode,
    resolve_platform_description,
)
import tools.runners.runner_renode as runner_renode


def test_renode_discovery(monkeypatch, tmp_path):
    # Test discovery via RENODE_PATH
    fake_renode = tmp_path / "fake_renode.exe"
    fake_renode.write_text("#!/bin/sh\n")
    monkeypatch.setenv("RENODE_PATH", str(fake_renode))
    assert find_renode_executable() == fake_renode

    # Test discovery when RENODE_PATH unset
    monkeypatch.delenv("RENODE_PATH", raising=False)
    # find_renode_executable should either find host Renode or return None
    discovered = find_renode_executable()
    if discovered:
        assert discovered.exists()


def test_build_resc_script_cortex_a53(tmp_path):
    kernel_elf = tmp_path / "kernel.elf"
    kernel_elf.write_text("dummy elf")

    dtb_file = tmp_path / "hw.dtb"
    dtb_file.write_text("dummy dtb")

    manifest = {
        "arch": "arm64",
        "run_config": {
            "machine": "cortex-a53-gicv2",
            "serial": ["uart0"],
        },
        "artifacts": {
            "boot_artifact": str(kernel_elf),
            "dtb_artifact": str(dtb_file),
        },
    }

    resc_file = tmp_path / "test.resc"
    content = build_resc_script(manifest, port=12345, repo_root=REPO_ROOT, script_path=resc_file)

    assert "mach create" in content
    assert "LoadPlatformDescription @platforms/cpus/cortex-a53-gicv2.repl" in content
    assert "sysbus LoadBinary" in content
    assert "sysbus LoadELF" in content
    assert "emulation CreateServerSocketTerminal 12345" in content
    assert "connector Connect sysbus.uart0 guest_uart_term" in content
    assert "start" in content


def test_build_resc_script_stm32f4(tmp_path):
    kernel_elf = tmp_path / "kernel.elf"
    kernel_elf.write_text("dummy elf")

    manifest = {
        "arch": "arm32",
        "run_config": {
            "machine": "stm32f4_discovery",
            "serial": ["usart2"],
        },
        "artifacts": {
            "boot_artifact": str(kernel_elf),
        },
    }

    resc_file = tmp_path / "stm32.resc"
    content = build_resc_script(manifest, port=54321, repo_root=REPO_ROOT, script_path=resc_file)

    assert "mach create" in content
    assert "LoadPlatformDescription @platforms/boards/stm32f4_discovery.repl" in content
    assert "sysbus LoadELF" in content
    assert "connector Connect sysbus.usart2 guest_uart_term" in content


def test_manifest_validation_rejects_missing_boot_artifact(tmp_path):
    manifest = {
        "target_name": "test_target",
        "runner_type": "renode",
        "run_config": {"machine": "cortex-a53-gicv2"},
        "artifacts": {},
    }
    assert run_renode(manifest) == 1


def test_manifest_validation_rejects_desktop_pci(tmp_path):
    kernel_elf = tmp_path / "kernel.elf"
    kernel_elf.write_text("dummy elf")

    manifest = {
        "target_name": "test_target",
        "runner_type": "renode",
        "device_list": ["pci-bridge", "virtio-gpu-pci"],
        "run_config": {"machine": "cortex-a53-gicv2"},
        "artifacts": {"boot_artifact": str(kernel_elf)},
    }
    assert run_renode(manifest) == 1


def test_run_renode_missing_executable_fails(tmp_path, monkeypatch):
    kernel_elf = tmp_path / "kernel.elf"
    kernel_elf.write_text("dummy elf")

    manifest = {
        "target_name": "test_target",
        "runner_type": "renode",
        "run_config": {"machine": "cortex-a53-gicv2"},
        "artifacts": {"boot_artifact": str(kernel_elf)},
    }
    monkeypatch.setattr(runner_renode, "find_renode_executable", lambda: None)
    assert run_renode(manifest) == 1
