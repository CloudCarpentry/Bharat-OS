from typing import Protocol


class FlashBackend(Protocol):
    def execute(self, manifest: dict, dry_run: bool = False) -> int: ...
