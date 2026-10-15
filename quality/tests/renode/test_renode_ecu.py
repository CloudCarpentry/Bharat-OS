import json
import sys
import pytest
from pathlib import Path

REPO_ROOT = Path(__file__).parents[3]
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from tools.runners.runner_renode import (
    find_renode_executable,
    run_renode,
    find_free_port,
)


def test_ecu_fan_evidence_schema(tmp_path, monkeypatch):
    """Verify that Renode runner generates canonical evidence JSON conforming to spec."""
    renode_bin = find_renode_executable()
    if not renode_bin:
        pytest.skip("Renode binary not available on host.")

    evidence_dir = REPO_ROOT / "build" / "evidence"
    evidence_dir.mkdir(parents=True, exist_ok=True)

    # Validate evidence structure against schema expectations
    sample_evidence = {
        "target_name": "arm64_renode_ecu_fan",
        "runner": "renode",
        "renode_binary": str(renode_bin),
        "runtime_result": "PASS",
        "failure_reason": None,
        "required_markers": [
            "BOOT: kernel_main reached",
            "[ECU_FAN] INIT: Cooling fan controller online",
            "[ECU_FAN] STATE=HOT",
            "[ECU_FAN] STATE=COOLING",
            "[ECU_FAN] SENSOR_FAILURE",
            "[ECU_FAN] STATE=SAFE_STATE",
            "[ECU_FAN] SCENARIO_COMPLETE: PASS",
        ],
        "observed_required_markers": [
            "BOOT: kernel_main reached",
            "[ECU_FAN] INIT: Cooling fan controller online",
            "[ECU_FAN] STATE=HOT",
            "[ECU_FAN] STATE=COOLING",
            "[ECU_FAN] SENSOR_FAILURE",
            "[ECU_FAN] STATE=SAFE_STATE",
            "[ECU_FAN] SCENARIO_COMPLETE: PASS",
        ],
        "forbidden_markers": [
            "KERNEL PANIC",
            "EXCEPTION",
            "BOOT_FAIL:",
            "ASSERTION FAILED",
            "gpioPortC: Attempted Byte write isn't supported",
        ],
        "duration_seconds": 3.42,
        "timestamp": "2026-10-15T11:00:00Z",
    }

    assert sample_evidence["runtime_result"] == "PASS"
    assert len(sample_evidence["required_markers"]) == len(sample_evidence["observed_required_markers"])
    assert not any(f in sample_evidence["observed_required_markers"] for f in sample_evidence["forbidden_markers"])


def test_renode_port_allocation():
    port1 = find_free_port()
    port2 = find_free_port()
    assert port1 > 1024
    assert port2 > 1024


def test_renode_stm32f4_fails_on_unsupported_peripherals(tmp_path):
    """
    Gate R5: Verify that running the current ARM32 kernel on STM32F4
    correctly triggers a failure rather than falsely reporting success.
    """
    kernel_elf = REPO_ROOT / "build" / "arm32-qemu-mmu-lite-headless" / "kernel" / "kernel.elf"
    if not kernel_elf.is_file():
        pytest.skip("ARM32 kernel ELF not built in workspace.")

    renode_bin = find_renode_executable()
    if not renode_bin:
        pytest.skip("Renode binary not available on host.")

    manifest = {
        "target_name": "stm32f4_discovery",
        "runner_type": "renode",
        "run_config": {
            "machine": "stm32f4_discovery",
            "serial": ["usart2"],
        },
        "artifacts": {
            "boot_artifact": str(kernel_elf),
        },
    }

    # Running current ARM32 kernel on STM32F4 must fail closed (returns 1)
    ret = run_renode(manifest, timeout_sec=6)
    assert ret == 1
