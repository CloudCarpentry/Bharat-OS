"""Unit tests for Bharat-OS board-aware linker script generation."""

import pytest
from pathlib import Path

from tools.board.manifest import load_board_manifest
from tools.board.linker_generator import generate_linker_script, resolve_template_name


def test_resolve_template_name():
    assert resolve_template_name("armv7e-m") == "cortex_m.ld.in"
    assert resolve_template_name("rv32-mcu") == "riscv32_mcu.ld.in"
    assert resolve_template_name("armv7-a") == "arm32_mmu_lite.ld.in"
    assert resolve_template_name("rv32-sv32") == "riscv32_sv32.ld.in"
    with pytest.raises(ValueError):
        resolve_template_name("unknown_arch")


def test_generate_cortex_m_linker(tmp_path):
    manifest = load_board_manifest("stm32f407_discovery")
    out_file = tmp_path / "stm32f4.ld"
    generate_linker_script(manifest, out_file)

    assert out_file.is_file()
    content = out_file.read_text(encoding="utf-8")

    assert "OUTPUT_FORMAT(\"elf32-littlearm\")" in content
    assert "FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K" in content
    assert "SRAM (rwx) : ORIGIN = 0x20000000, LENGTH = 128K" in content
    assert ".isr_vector" in content
    assert "ASSERT(__static_end <= __stack_limit" in content


def test_generate_riscv_mcu_linker(tmp_path):
    manifest = load_board_manifest("hifive1_rev_b")
    out_file = tmp_path / "hifive1.ld"
    generate_linker_script(manifest, out_file)

    assert out_file.is_file()
    content = out_file.read_text(encoding="utf-8")

    assert "OUTPUT_FORMAT(\"elf32-littleriscv\")" in content
    assert "FLASH (rx) : ORIGIN = 0x20000000, LENGTH = 4096K" in content
    assert "DTIM (rwx) : ORIGIN = 0x80000000, LENGTH = 16K" in content
    assert ".init" in content
