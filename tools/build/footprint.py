import csv
from pathlib import Path
from typing import Optional

from tools.build.models import ResolvedTarget


def _parse_memory_to_kb(memory: Optional[str]) -> Optional[int]:
    if not memory:
        return None
    value = memory.strip().upper()
    if value.endswith("G"):
        return int(value[:-1]) * 1024 * 1024
    if value.endswith("M"):
        return int(value[:-1]) * 1024
    if value.endswith("K"):
        return int(value[:-1])
    if value.isdigit():
        return int(value) // 1024
    return None


def validate_footprint_contract(target: ResolvedTarget, repo_root: Path) -> None:
    matrix_path = repo_root / "configs" / "footprint" / "footprint_matrix.csv"
    if not matrix_path.exists():
        return

    if not target.footprint_profile:
        return

    selected = None
    with open(matrix_path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row.get("profile_id") == target.footprint_profile:
                selected = row
                break

    if not selected:
        raise ValueError(
            f"Unknown footprint_profile '{target.footprint_profile}' for target '{target.name}'."
        )

    if selected.get("arch") != target.arch:
        raise ValueError(
            f"Footprint profile arch mismatch for target '{target.name}': "
            f"profile arch={selected.get('arch')} target arch={target.arch}."
        )

    run_memory_kb = _parse_memory_to_kb(target.run.memory if target.run else None)
    required_boot_kb = int(selected.get("boot_min_ram_kb", "0"))
    if run_memory_kb is not None and run_memory_kb < required_boot_kb:
        raise ValueError(
            f"Target '{target.name}' memory ({run_memory_kb} KB) is below boot minimum "
            f"({required_boot_kb} KB) for profile '{target.footprint_profile}'."
        )
