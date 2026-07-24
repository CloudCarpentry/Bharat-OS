#!/usr/bin/env bash
# tools/testing/test_check_boot_log.sh - Test suite for the boot log parser

set -u

PARSER="tools/testing/check_boot_log.py"
CONTRACT="quality/contracts/boot/headless_boot_contract.yaml"
FIXTURE_DIR="quality/fixtures/boot"

PASS_COUNT=0
FAIL_COUNT=0

run_test() {
    local name="$1"
    local target="$2"
    local log="$3"
    local expected_exit="$4"
    local extra_args="${5:-}"

    echo -n "Running test: $name... "

    # Run parser and capture output
    output=$(python3 "$PARSER" --target "$target" --log "$log" --contract "$CONTRACT" $extra_args 2>&1)
    exit_code=$?

    if [ $exit_code -eq $expected_exit ]; then
        echo "PASS (Exit $exit_code)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        echo "FAIL (Expected $expected_exit, got $exit_code)"
        echo "--- Output ---"
        echo "$output"
        echo "--------------"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
}

echo "=== Boot Log Parser Test Suite ==="

# 1. Valid log passes
run_test "Valid x86_64 log" "x86_64_desktop_headless" "$FIXTURE_DIR/pass_x86_64_headless.log" 0

# 2. Panic log fails
run_test "Panic log" "x86_64_desktop_headless" "$FIXTURE_DIR/fail_panic.log" 1

# 3. Missing required marker fails
run_test "Missing required marker" "x86_64_desktop_headless" "$FIXTURE_DIR/fail_missing_required.log" 1

# 4. Allowed skip does not fail
run_test "Allowed skip" "arm32_mmu_lite_headless" "$FIXTURE_DIR/pass_single_core_skip.log" 0

# 5. Forbidden marker fails (already covered by panic, but good to have)
run_test "Forbidden marker" "x86_64_desktop_headless" "$FIXTURE_DIR/fail_panic.log" 1

# 6. Unknown target fails closed (exit 2)
run_test "Unknown target" "unknown_target_999" "$FIXTURE_DIR/pass_x86_64_headless.log" 2

# 7. Strict mode: suspicious marker fails
run_test "Strict mode: suspicious marker" "x86_64_desktop_headless" "$FIXTURE_DIR/fail_unknown_error_strict.log" 1 "--strict"

# 8. Strict mode: pass with allowed skip
run_test "Strict mode: allow skip" "arm32_mmu_lite_headless" "$FIXTURE_DIR/pass_single_core_skip.log" 0 "--strict"

echo "=================================="
echo "Summary: PASS=$PASS_COUNT FAIL=$FAIL_COUNT"
echo "=================================="

if [ $FAIL_COUNT -ne 0 ]; then
    exit 1
fi
exit 0
