#!/usr/bin/env python3
"""
Board Manifest Parser and Validator for Bharat-OS.
"""

from __future__ import annotations

import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
if str(REPO_ROOT) not in sys.path:
    sys.path.insert(0, str(REPO_ROOT))

from typing import Any, Dict, List, Optional
import yaml
import jsonschema


def parse_size(val: int | str) -> int:
    """Parse integer or human-readable size string (e.g., '1M', '128K', '4K', '0x20000')."""
    if isinstance(val, int):
        return val
    val_str = str(val).strip()
    if val_str.startswith(("0x", "0X")):
        return int(val_str, 16)
    
    match = re.match(r"^(\d+)\s*([KkMmGg])?[Bb]?$", val_str)
    if not match:
        return int(val_str)
    
    num = int(match.group(1))
    unit = match.group(2)
    if not unit:
        return num
    unit = unit.upper()
    if unit == "K":
        return num * 1024
    if unit == "M":
        return num * 1024 * 1024
    if unit == "G":
        return num * 1024 * 1024 * 1024
    return num


def parse_address(val: int | str) -> int:
    """Parse memory address into integer."""
    if isinstance(val, int):
        return val
    val_str = str(val).strip()
    if val_str.startswith(("0x", "0X")):
        return int(val_str, 16)
    return int(val_str)


@dataclass
class MemoryRegion:
    name: str
    origin: int
    length: int
    executable: bool = False
    writable: bool = False
    type: str = "sram"

    @property
    def end(self) -> int:
        return self.origin + self.length


@dataclass
class MmioRegion:
    name: str
    origin: int
    length: int
    description: str = ""

    @property
    def end(self) -> int:
        return self.origin + self.length


@dataclass
class RuntimeConfig:
    kernel_stack: int
    application_stack: int
    heap: int
    mode: str


@dataclass
class ProtectionConfig:
    model: str


@dataclass
class BootConfig:
    entry: str
    vector_location: str
    vector_alignment: int = 4


@dataclass
class DeviceConfig:
    console: str
    timer: str
    interrupt_controller: str
    uart_base: Optional[int] = None


@dataclass
class BoardManifest:
    board: str
    soc: str
    architecture: str
    cpu: str
    fpu_abi: str
    memory: Dict[str, MemoryRegion]
    mmio: List[MmioRegion]
    runtime: RuntimeConfig
    protection: ProtectionConfig
    boot: BootConfig
    devices: DeviceConfig
    raw_data: Dict[str, Any] = field(default_factory=dict)
    manifest_path: Optional[Path] = None

    def find_executable_region(self) -> Optional[MemoryRegion]:
        for reg in self.memory.values():
            if reg.executable:
                return reg
        return None

    def find_writable_region(self) -> Optional[MemoryRegion]:
        for reg in self.memory.values():
            if reg.writable:
                return reg
        return None

    def get_flash_region(self) -> Optional[MemoryRegion]:
        if "flash" in self.memory:
            return self.memory["flash"]
        return self.find_executable_region()

    def get_ram_region(self) -> Optional[MemoryRegion]:
        for candidate in ("sram", "dtim", "ram"):
            if candidate in self.memory:
                return self.memory[candidate]
        return self.find_writable_region()


def load_board_manifest(path_or_name: str | Path, repo_root: Optional[Path] = None) -> BoardManifest:
    """
    Load and validate a board manifest from a YAML file or board name.
    """
    if repo_root is None:
        repo_root = Path(__file__).resolve().parent.parent.parent

    path = Path(path_or_name)
    if not path.is_file():
        # Try looking up platform/boards/<board_name>/board.yaml
        candidate = repo_root / "platform" / "boards" / str(path_or_name) / "board.yaml"
        if candidate.is_file():
            path = candidate
        else:
            raise FileNotFoundError(f"Board manifest not found for '{path_or_name}'. Checked: {path}, {candidate}")

    with open(path, "r", encoding="utf-8") as f:
        data = yaml.safe_load(f)

    schema_path = repo_root / "tools" / "schemas" / "board.schema.yaml"
    if schema_path.is_file():
        with open(schema_path, "r", encoding="utf-8") as sf:
            schema = yaml.safe_load(sf)
        jsonschema.validate(instance=data, schema=schema)

    # Parse typed fields
    memory_regions = {}
    for name, mdata in data.get("memory", {}).items():
        memory_regions[name] = MemoryRegion(
            name=name,
            origin=parse_address(mdata["origin"]),
            length=parse_size(mdata["length"]),
            executable=bool(mdata.get("executable", False)),
            writable=bool(mdata.get("writable", False)),
            type=str(mdata.get("type", "sram")),
        )

    mmio_regions = []
    for mdata in data.get("mmio", []):
        mmio_regions.append(
            MmioRegion(
                name=mdata["name"],
                origin=parse_address(mdata["origin"]),
                length=parse_size(mdata["length"]),
                description=mdata.get("description", ""),
            )
        )

    r_data = data["runtime"]
    runtime = RuntimeConfig(
        kernel_stack=parse_size(r_data.get("kernel_stack", "4K")),
        application_stack=parse_size(r_data.get("application_stack", "2K")),
        heap=parse_size(r_data.get("heap", "0")),
        mode=r_data.get("mode", "static"),
    )

    p_data = data["protection"]
    protection = ProtectionConfig(model=p_data["model"])

    b_data = data["boot"]
    boot = BootConfig(
        entry=b_data["entry"],
        vector_location=b_data["vector_location"],
        vector_alignment=b_data.get("vector_alignment", 4),
    )

    d_data = data["devices"]
    uart_base = parse_address(d_data["uart_base"]) if "uart_base" in d_data else None
    devices = DeviceConfig(
        console=d_data["console"],
        timer=d_data["timer"],
        interrupt_controller=d_data["interrupt_controller"],
        uart_base=uart_base,
    )

    return BoardManifest(
        board=data["board"],
        soc=data["soc"],
        architecture=data["architecture"],
        cpu=data["cpu"],
        fpu_abi=data.get("fpu_abi", "soft"),
        memory=memory_regions,
        mmio=mmio_regions,
        runtime=runtime,
        protection=protection,
        boot=boot,
        devices=devices,
        raw_data=data,
        manifest_path=path,
    )
