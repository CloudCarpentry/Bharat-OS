"""Unit and integration tests for Bharat-OS board qualification tool (validate.py)."""

import pytest
from tools.board.manifest import load_board_manifest, MemoryRegion
from tools.board.validate import BoardValidator


def test_validate_stm32f407():
    manifest = load_board_manifest("stm32f407_discovery")
    validator = BoardValidator(manifest)
    assert validator.validate_all() is True
    # Verify all checks passed
    for name, passed, detail in validator.results:
        assert passed is True, f"Check '{name}' unexpectedly failed: {detail}"


def test_validate_hifive1():
    manifest = load_board_manifest("hifive1_rev_b")
    validator = BoardValidator(manifest)
    assert validator.validate_all() is True
    for name, passed, detail in validator.results:
        assert passed is True, f"Check '{name}' unexpectedly failed: {detail}"


def test_catch_mmio_overlap():
    manifest = load_board_manifest("stm32f407_discovery")
    # Deliberately inject the buggy 0x40010000 memory region that collided with TIM1
    manifest.memory["buggy_ram"] = MemoryRegion(
        name="buggy_ram",
        origin=0x40010000,
        length=65536,
        executable=True,
        writable=True,
        type="sram",
    )
    validator = BoardValidator(manifest)
    assert validator.check_no_mmio_overlap() is False
    # Check failure message
    failed_check = [r for r in validator.results if r[0] == "No MMIO overlap"]
    assert len(failed_check) == 1
    assert "overlaps MMIO" in failed_check[0][2]


def test_catch_sram_overflow():
    manifest = load_board_manifest("stm32f407_discovery")
    # Deliberately request 256KB stack on a 128KB SRAM device
    manifest.runtime.kernel_stack = 256 * 1024
    validator = BoardValidator(manifest)
    assert validator.check_sram_stack() is False
    failed_check = [r for r in validator.results if r[0] == "SRAM and reserved stack fit"]
    assert len(failed_check) == 1
    assert "exceed RAM size" in failed_check[0][2]


def test_catch_cpu_abi_mismatch():
    manifest = load_board_manifest("stm32f407_discovery")
    # Deliberately set Cortex-A15 on ARMv7E-M architecture
    manifest.cpu = "cortex-a15"
    validator = BoardValidator(manifest)
    assert validator.check_cpu_abi() is False
