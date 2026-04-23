from __future__ import annotations

from pathlib import Path
from typing import Iterable

REPO_ROOT = Path(__file__).resolve().parents[2]


_TARGET_ALIASES: tuple[tuple[str, str], ...] = (
    ("tools/targets/qemu/", "delivery/targets/qemu/"),
)

_TARGET_MATRIX_ALIASES: tuple[tuple[str, str], ...] = (
    ("targets/target_matrix.json", "delivery/targets/target_matrix.json"),
)


def resolve_migration_alias(path: Path, aliases: tuple[tuple[str, str], ...]) -> tuple[Path, bool]:
    """Resolve repository path aliases during incremental migrations.

    Returns: (resolved_path, used_alias)
    """
    if path.exists():
        return path, False

    raw = str(path).replace("\\", "/")
    candidate_paths: list[Path] = []

    for legacy_prefix, new_prefix in aliases:
        if legacy_prefix in raw:
            candidate_paths.append(Path(raw.replace(legacy_prefix, new_prefix, 1)))
        if new_prefix in raw:
            candidate_paths.append(Path(raw.replace(new_prefix, legacy_prefix, 1)))

    for candidate in candidate_paths:
        candidate_abs = candidate if candidate.is_absolute() else (REPO_ROOT / candidate)
        if candidate_abs.exists():
            return candidate_abs, True

    return path, False


def resolve_target_yaml_alias(path: Path) -> tuple[Path, bool]:
    return resolve_migration_alias(path, _TARGET_ALIASES)


def resolve_target_matrix_alias(path: Path) -> tuple[Path, bool]:
    return resolve_migration_alias(path, _TARGET_MATRIX_ALIASES)


def repo_path_candidates(*relative_paths: str) -> list[Path]:
    return [REPO_ROOT / rel for rel in relative_paths]


def first_existing_path(candidates: Iterable[Path]) -> Path | None:
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None
