#!/usr/bin/env python3
"""Validate the POSIX portability-layer contract and staged MVP inventory."""

from pathlib import Path

import yaml


CONTRACT = Path(__file__).parents[2] / "contracts" / "posix_subset_v1.yaml"
PROJECT_ROOT = CONTRACT.parent.parent

EXPECTED_IMPLEMENTED = {"read", "write", "close"}
EXPECTED_STAGES = {
    1: {
        "read", "write", "close", "open", "openat", "lseek", "fstat",
        "isatty", "clock_gettime", "nanosleep", "getpid",
    },
    2: {"mmap", "munmap", "mprotect", "brk", "sbrk", "_exit"},
    3: {
        "pthread_create", "pthread_join", "pthread_mutex", "pthread_cond", "TLS",
    },
    4: set(),
}


def main() -> None:
    contract = yaml.safe_load(CONTRACT.read_text(encoding="utf-8"))["posix_subset"]

    assert contract["version"] == 1
    assert contract["kind"] == "source_api_portability_layer"
    assert contract["raw_kernel_syscall_namespace"] is False
    assert set(contract["implemented"]) == EXPECTED_IMPLEMENTED

    stages = {entry["stage"]: set(entry["functions"]) for entry in contract["stages"]}
    assert stages == EXPECTED_STAGES
    assert set(contract["functions"]) == set().union(*EXPECTED_STAGES.values())
    assert EXPECTED_IMPLEMENTED <= stages[1]

    socket_stage = next(entry for entry in contract["stages"] if entry["stage"] == 4)
    assert socket_stage["prerequisite"] == "stable_network_service_abi"

    for profile_name in ("edge_posix", "hosted_desktop"):
        profile_path = PROJECT_ROOT / "profiles" / f"{profile_name}.yaml"
        profile = yaml.safe_load(profile_path.read_text(encoding="utf-8"))
        assert profile["personality"] == "NATIVE"
        assert profile["features"]["posix_io"] is True


if __name__ == "__main__":
    main()
