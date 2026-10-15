"""Unit tests for Bharat-OS board manifest parsing and schema validation."""

import pytest
from pathlib import Path

from tools.board.manifest import load_board_manifest, parse_size, parse_address


def test_parse_size():
    assert parse_size(1024) == 1024
    assert parse_size("1024") == 1024
    assert parse_size("4K") == 4096
    assert parse_size("128K") == 131072
    assert parse_size("1M") == 1048576
    assert parse_size("4M") == 4194304
    assert parse_size("0x20000") == 131072


def test_parse_address():
    assert parse_address(0x08000000) == 0x08000000
    assert parse_address("0x08000000") == 0x08000000
    assert parse_address("0x20000000") == 0x20000000


def test_load_stm32f407_manifest():
    manifest = load_board_manifest("stm32f407_discovery")
    assert manifest.board == "stm32f407_discovery"
    assert manifest.architecture == "armv7e-m"
    assert manifest.cpu == "cortex-m4"
    assert "flash" in manifest.memory
    assert manifest.memory["flash"].origin == 0x08000000
    assert manifest.memory["flash"].length == 1048576
    assert manifest.memory["flash"].executable is True
    assert "sram" in manifest.memory
    assert manifest.memory["sram"].origin == 0x20000000
    assert manifest.memory["sram"].length == 131072
    assert manifest.memory["sram"].writable is True
    assert manifest.protection.model == "MPU_ONLY"
    assert len(manifest.mmio) >= 3


def test_load_hifive1_manifest():
    manifest = load_board_manifest("hifive1_rev_b")
    assert manifest.board == "hifive1_rev_b"
    assert manifest.architecture == "rv32-mcu"
    assert manifest.cpu == "sifive-e31"
    assert "flash" in manifest.memory
    assert manifest.memory["flash"].origin == 0x20000000
    assert "dtim" in manifest.memory
    assert manifest.memory["dtim"].origin == 0x80000000
    assert manifest.protection.model == "PMP_ONLY"
