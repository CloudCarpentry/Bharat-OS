import sys
import subprocess
import os

def run_test(name, log_content, expected_exit):
    print(f"Running test: {name}...", end=" ")

    with open("temp_log.txt", "w") as f:
        f.write(log_content)

    cmd = [
        "python3", "tools/testing/check_boot_log.py",
        "--target", "x86_64_desktop_headless",
        "--log", "temp_log.txt",
        "--strict",
        "--contract", "quality/contracts/boot/headless_boot_contract.yaml"
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode == expected_exit:
        print(f"PASS (Exit {result.returncode})")
        return True
    else:
        print(f"FAIL (Expected {expected_exit}, got {result.returncode})")
        print(result.stdout)
        return False

base_log = """
BOOT_HANDOFF: NORMALIZED
BOOT_HANDOFF: VALIDATED
BOOT_MEMORY: MODULES_RESERVED
INIT_MODULE: services/init FOUND
INIT_ELF: VALIDATED
INIT_ASPACE: READY
INIT_THREAD: SCHEDULED
USER_INIT: ENTERED
USER_INIT: STARTUP_ABI_OK
NAMESVC_USER_ENTRY
NAMESVC_MAIN_ENTER
NAMESVC_READY
PROCESS_MANAGER_LAUNCH
PROCESS_MANAGER_READY
BOOT_CLASS_CORE_READY
USER_INIT: SERVICE_GRAPH_COMPLETE
"""

tests = []

valid_log = base_log + """
BOOT_EVT v=1 session=42 seq=10 service=namesvc incarnation=1 event=SPAWNED
BOOT_EVT v=1 session=42 seq=11 service=namesvc incarnation=1 event=BOUND
BOOT_EVT v=1 session=42 seq=12 service=namesvc incarnation=1 event=READY
BOOT_EVT v=1 session=42 seq=13 service=namesvc incarnation=1 event=HEALTHY
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Valid structured log", valid_log, 0))

dup_spawn_log = base_log + """
BOOT_EVT v=1 session=42 seq=10 service=namesvc incarnation=1 event=SPAWNED
BOOT_EVT v=1 session=42 seq=11 service=namesvc incarnation=1 event=SPAWNED
BOOT_EVT v=1 session=42 seq=12 service=namesvc incarnation=1 event=BOUND
BOOT_EVT v=1 session=42 seq=13 service=namesvc incarnation=1 event=READY
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Duplicate spawn", dup_spawn_log, 1))

ready_no_bound_log = base_log + """
BOOT_EVT v=1 session=42 seq=10 service=namesvc incarnation=1 event=SPAWNED
BOOT_EVT v=1 session=42 seq=12 service=namesvc incarnation=1 event=READY
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Ready without bound", ready_no_bound_log, 1))

ready_no_spawn_log = base_log + """
BOOT_EVT v=1 session=42 seq=12 service=namesvc incarnation=1 event=READY
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Ready without spawn", ready_no_spawn_log, 1))

stale_ready_log = base_log + """
BOOT_EVT v=1 session=42 seq=10 service=namesvc incarnation=2 event=SPAWNED
BOOT_EVT v=1 session=42 seq=11 service=namesvc incarnation=1 event=BOUND
BOOT_EVT v=1 session=42 seq=12 service=namesvc incarnation=1 event=READY
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Stale ready", stale_ready_log, 1))

missing_ready_log = base_log + """
BOOT_EVT v=1 session=42 seq=10 service=namesvc incarnation=1 event=SPAWNED
BOOT_EVT v=1 session=42 seq=11 service=namesvc incarnation=1 event=BOUND
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Missing ready", missing_ready_log, 1))

mixed_sessions_log = base_log + """
BOOT_EVT v=1 session=42 seq=10 service=namesvc incarnation=1 event=SPAWNED
BOOT_EVT v=1 session=42 seq=11 service=namesvc incarnation=1 event=BOUND
BOOT_EVT v=1 session=43 seq=12 service=namesvc incarnation=1 event=READY
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Mixed sessions", mixed_sessions_log, 1))

missing_events_stable_strict_log = base_log + """
BOOT_RUNTIME: STABLE
"""
tests.append(("Missing events strict stable", missing_events_stable_strict_log, 1))

crashed_unrecovered_log = base_log + """
BOOT_EVT v=1 session=42 seq=10 service=namesvc incarnation=1 event=SPAWNED
BOOT_EVT v=1 session=42 seq=11 service=namesvc incarnation=1 event=BOUND
BOOT_EVT v=1 session=42 seq=12 service=namesvc incarnation=1 event=READY
BOOT_EVT v=1 session=42 seq=13 service=namesvc incarnation=1 event=CRASHED
BOOT_EVT v=1 session=42 seq=30 event=STABLE
BOOT_RUNTIME: STABLE
"""
tests.append(("Crashed unrecovered", crashed_unrecovered_log, 1))

all_passed = True
for name, log, expected in tests:
    if not run_test(name, log, expected):
        all_passed = False

if os.path.exists("temp_log.txt"):
    os.remove("temp_log.txt")

if not all_passed:
    sys.exit(1)
