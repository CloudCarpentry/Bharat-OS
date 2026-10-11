"""Compile production paths with fake hardware; keep security assertions active.

Run: python3 -m pytest quality/tests/security/test_cap_revocation_mapping_audit.py -ra
Known integration gaps use strict xfail: a fix produces XPASS and requires review.
Compiler/setup errors are fixture errors, never expected security failures.
"""
from pathlib import Path
import os
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[3]


class MappingCompletionGap(AssertionError):
    """Only the explicit unsafe-observation exit code is an expected failure."""


@pytest.fixture(scope="session")
def audit_binary(tmp_path_factory):
    subprocess.run(["cmake", "--preset", "x86_64-dev"], cwd=ROOT, check=True,
                   capture_output=True, text=True)
    output = tmp_path_factory.mktemp("cap-p0-004") / "audit"
    includes = ["build/x86_64-dev/generated/include", "core/kernel/include",
                "core/hal/include", "core/lib/include", "core/lib/cap/include",
                "core/personalities/runtime/include", "core/boot/include",
                "interface/include", "interface"]
    sources = ["quality/tests/security/cap_revocation_mapping_audit.c",
               "core/kernel/src/cap/cap_cspace.c", "core/kernel/src/cap/cap_dispatch.c",
               "core/kernel/src/cap/cap_revoke.c", "core/kernel/src/cap/cap_policy.c",
               "core/kernel/src/ds/bh_id_allocator.c", "core/kernel/src/mm/vmm.c",
               "core/kernel/src/mm/dma/dma.c", "core/kernel/src/mm/dma/dma_grant.c"]
    command = [os.environ.get("CC", "clang"), "-std=c11", "-g", "-O0",
               "-DBHARAT_HOST_TEST=1", "-ffunction-sections", "-fdata-sections",
               "-Wl,--gc-sections", "-o", str(output)]
    command += [f"-I{ROOT / path}" for path in includes]
    command += [str(ROOT / path) for path in sources]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    return output


def run_case(binary, case):
    result = subprocess.run([str(binary), case], capture_output=True, text=True, timeout=10)
    # Harness assertion failure/signal is not an accepted integration-gap result.
    assert result.returncode in (0, 1), result.stdout + result.stderr
    return result


@pytest.mark.parametrize("case", ["stale", "remote-stale"])
def test_stale_and_reused_capabilities(audit_binary, case):
    result = run_case(audit_binary, case)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("case", ["cpu", "inflight", "dma", "grant-failure"])
@pytest.mark.xfail(strict=True, raises=MappingCompletionGap,
                   reason="CAP-P0-004: mapping completion contract not integrated")
def test_revoke_disables_established_access(audit_binary, case):
    result = run_case(audit_binary, case)
    if result.returncode == 1:
        raise MappingCompletionGap(result.stdout + result.stderr)
