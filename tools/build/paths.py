from pathlib import Path

from tools.build.models import ResolvedTarget


def get_output_root(target: ResolvedTarget, repo_root: Path) -> Path:
    preset = target.build.cmake_preset
    return repo_root / "build" / preset


def get_manifest_dir(target: ResolvedTarget, repo_root: Path) -> Path:
    return get_output_root(target, repo_root) / "manifests"


def get_packaged_dir(target: ResolvedTarget, repo_root: Path) -> Path:
    return get_output_root(target, repo_root) / "packaged"


def get_logs_dir(target: ResolvedTarget, repo_root: Path) -> Path:
    return get_output_root(target, repo_root) / "logs"
