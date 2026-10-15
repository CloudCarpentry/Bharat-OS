#!/usr/bin/env python3
"""
Board-Aware Linker Script Generator for Bharat-OS.
Renders target linker scripts from declarative board manifests and architecture templates.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from typing import Optional

from jinja2 import Environment, FileSystemLoader

from tools.board.manifest import BoardManifest, load_board_manifest


def resolve_template_name(arch: str) -> str:
    """Map board architecture to the canonical linker template."""
    arch_map = {
        "armv7e-m": "cortex_m.ld.in",
        "rv32-mcu": "riscv32_mcu.ld.in",
        "armv7-a": "arm32_mmu_lite.ld.in",
        "rv32-sv32": "riscv32_sv32.ld.in",
    }
    if arch not in arch_map:
        raise ValueError(f"No linker template mapped for architecture '{arch}'. Available: {list(arch_map.keys())}")
    return arch_map[arch]


def generate_linker_script(
    manifest: BoardManifest,
    output_path: Path,
    templates_dir: Optional[Path] = None,
    repo_root: Optional[Path] = None,
) -> Path:
    """
    Render linker script from manifest and template.
    """
    if repo_root is None:
        repo_root = Path(__file__).resolve().parent.parent.parent
    if templates_dir is None:
        templates_dir = repo_root / "delivery" / "linker" / "templates"

    template_filename = resolve_template_name(manifest.architecture)
    env = Environment(loader=FileSystemLoader(str(templates_dir)), trim_blocks=True, lstrip_blocks=True)
    template = env.get_template(template_filename)

    # Helper context variables
    flash_reg = manifest.get_flash_region()
    ram_reg = manifest.get_ram_region()
    flash_name = flash_reg.name if flash_reg else "FLASH"
    ram_name = ram_reg.name if ram_reg else "SRAM"

    # Context dict
    context = {
        "board": manifest,
        "flash_region_name": flash_name,
        "ram_region_name": ram_name,
        "flash_origin": f"0x{flash_reg.origin:08X}" if flash_reg else "0x00000000",
        "flash_length": f"{flash_reg.length // 1024}K" if flash_reg else "0K",
        "ram_origin": f"0x{ram_reg.origin:08X}" if ram_reg else "0x00000000",
        "ram_length": f"{ram_reg.length // 1024}K" if ram_reg else "0K",
    }

    rendered = template.render(context)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(rendered)

    # Also emit machine-readable board_layout.json alongside the generated script
    layout_meta = {
        "board": manifest.board,
        "soc": manifest.soc,
        "architecture": manifest.architecture,
        "cpu": manifest.cpu,
        "memory_regions": {
            name: {
                "origin": f"0x{reg.origin:08X}",
                "length": reg.length,
                "end": f"0x{reg.end:08X}",
                "executable": reg.executable,
                "writable": reg.writable,
            }
            for name, reg in manifest.memory.items()
        },
        "mmio_zones": [
            {
                "name": mm.name,
                "origin": f"0x{mm.origin:08X}",
                "length": mm.length,
                "end": f"0x{mm.end:08X}",
                "description": mm.description,
            }
            for mm in manifest.mmio
        ],
        "runtime": {
            "kernel_stack": manifest.runtime.kernel_stack,
            "application_stack": manifest.runtime.application_stack,
            "mode": manifest.runtime.mode,
        },
        "generated_linker": str(output_path),
    }

    layout_json_path = output_path.parent / f"{manifest.board}_layout.json"
    with open(layout_json_path, "w", encoding="utf-8") as f:
        json.dump(layout_meta, f, indent=2)

    return output_path


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate linker script from board manifest.")
    parser.add_argument("--board", required=True, help="Board manifest path or name under platform/boards/")
    parser.add_argument("--output", "-o", required=True, help="Output .ld file path")
    parser.add_argument("--templates-dir", help="Directory containing linker templates")
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parent.parent.parent
    try:
        manifest = load_board_manifest(args.board, repo_root=repo_root)
        out_path = Path(args.output)
        templates_dir = Path(args.templates_dir) if args.templates_dir else None
        generated = generate_linker_script(manifest, out_path, templates_dir=templates_dir, repo_root=repo_root)
        print(f"[Linker Generator] Successfully generated linker script: {generated}")
        return 0
    except Exception as exc:
        print(f"[Linker Generator] ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
