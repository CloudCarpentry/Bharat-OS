import pytest
from pathlib import Path

def check_log_against_contract(log_lines, required_markers, forbidden_markers):
    observed_required = set()
    failure_observed = False
    failure_reason = None

    for line in log_lines:
        # Check forbidden
        for forbidden in forbidden_markers:
            if forbidden in line:
                failure_observed = True
                failure_reason = f"Forbidden marker '{forbidden}' found: {line.strip()}"
                break
        if failure_observed:
            break

        # Check required
        for req in required_markers:
            if req not in observed_required and req in line:
                observed_required.add(req)

    contract_satisfied = len(observed_required) == len(required_markers)
    return contract_satisfied and not failure_observed, failure_reason

def test_contract_complete_success():
    required = ["BOOT: kernel_main reached", "BOOT: pmm initialized", "[BOOT] Runtime initialization complete"]
    forbidden = ["PANIC", "ASSERT", "FAULT"]

    logs = [
        "BOOT: kernel_main reached",
        "[INFO]  HAL: x86_64: Discovery initialized.",
        "BOOT: pmm initialized",
        "[BOOT] Runtime initialization complete",
        "[BOOT] Spawning first system service"
    ]

    passed, reason = check_log_against_contract(logs, required, forbidden)
    assert passed is True
    assert reason is None

def test_contract_missing_required():
    required = ["BOOT: kernel_main reached", "BOOT: pmm initialized", "[BOOT] Runtime initialization complete"]
    forbidden = ["PANIC", "ASSERT", "FAULT"]

    logs = [
        "BOOT: kernel_main reached",
        "[INFO]  HAL: x86_64: Discovery initialized.",
        "BOOT: pmm initialized"
    ]

    passed, reason = check_log_against_contract(logs, required, forbidden)
    assert passed is False
    assert reason is None

def test_contract_forbidden_found():
    required = ["BOOT: kernel_main reached", "BOOT: pmm initialized", "[BOOT] Runtime initialization complete"]
    forbidden = ["PANIC", "ASSERT", "FAULT"]

    logs = [
        "BOOT: kernel_main reached",
        "BOOT: pmm initialized",
        "PANIC: translation fault",
        "[BOOT] Runtime initialization complete"
    ]

    passed, reason = check_log_against_contract(logs, required, forbidden)
    assert passed is False
    assert "Forbidden marker 'PANIC' found" in reason
