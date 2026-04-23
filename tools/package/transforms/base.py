from __future__ import annotations

from typing import Protocol

from tools.build.models import ArtifactRecord, BuildOutputs, PackageTransformConfig, ResolvedTarget


class PackageTransform(Protocol):
    def name(self) -> str: ...
    def can_apply(self, target: ResolvedTarget, cfg: PackageTransformConfig, build_outputs: BuildOutputs) -> bool: ...
    def apply(self, target: ResolvedTarget, cfg: PackageTransformConfig, build_outputs: BuildOutputs) -> list[ArtifactRecord]: ...
