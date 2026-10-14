---
name: bharat-os-host-testing
description: Apply when writing, debugging, or running host-based unit tests, mock harnesses, CTest suites, or fast-feedback regression tests without full QEMU boots.
---

# Bharat-OS Host Testing Skill

## Role & Value of Host Testing

Host-based testing provides a **sub-5-second rapid feedback loop** for developers and autonomous agents. Instead of packaging full disk images and booting QEMU emulators, host tests compile kernel subsystems, data structures, and services directly on the host machine (Linux, macOS, or Windows) using the host C compiler (GCC, Clang, or MSVC).

Host tests are defined in [`quality/tests/host/`](file:///c:/workspace/Crazy_idea/Bharat-OS/quality/tests/host) and wired via [`quality/tests/host/CMakeLists.txt`](file:///c:/workspace/Crazy_idea/Bharat-OS/quality/tests/host/CMakeLists.txt).

## Core Mechanisms: `BHARAT_HOST_TEST=1`

Compiling with `-DBHARAT_HOST_TEST=1` replaces freestanding hardware dependencies with host mocks:
- **Mock HAL & Clock**: Ticks and monotonic time (`bh_ktime_now()`) advance deterministically under test control without hardware timers.
- **Mock Interrupts**: `quality/tests/host/test_irq_host.c` simulates IRQ assert/deassert and interrupt controller state machines.
- **Synthetic Multi-Core Concurrency**: Uses POSIX `pthreads` or Windows native threads to simulate multiple cores concurrently exercising TLB shootdowns, scheduler runqueues, and PMM pcache magazines.
- **Mock Panic Traps**: `quality/tests/host/mock_panic.c` catches kernel assertions and panics, verifying fail-closed error paths without crashing the test runner.

## Available Host Test Suites

| Subsystem Area | Key Host Test Binary | Test File |
|---|---|---|
| **TLB Shootdown** | `test_tlb_shootdown` | `quality/tests/host/test_tlb_shootdown.c` |
| **TLB Stress** | `test_tlb_stress` | `quality/tests/host/test_tlb_stress.c` |
| **PMM Per-Core Cache** | `test_pmm_pcache` | `quality/tests/host/test_pmm_pcache.c` |
| **PMM Remote Free** | `test_pmm_remote_free` | `quality/tests/host/test_pmm_remote_free.c` |
| **Capabilities** | `test_capability_ownership` | `quality/tests/host/test_capability_ownership.c` |
| **Scheduler SMP** | `test_sched` | `quality/tests/host/test_sched.c` |
| **Virtual Memory** | `test_vm_manager` | `quality/tests/host/test_vm_manager.c` |
| **Process Manager** | `test_process_manager` | `quality/tests/host/test_process_manager.c` |
| **Service Manager** | `test_servicemgr` | `quality/tests/host/test_servicemgr.c` |
| **UI Surface Stress** | `test_ui_surface_buffer_stress` | `quality/tests/host/test_ui_surface_buffer_stress.c` |
| **Device Tree / FDT** | `test_fdt_parser` | `quality/tests/host/test_fdt_parser.c` |

## How to Execute Host Tests

1. **Via Automated Agent Verifier**:
   ```bash
   python3 tools/agents/verify.py --host-tests
   ```

2. **Via CMake / CTest**:
   ```bash
   # Configure host tests preset
   cmake --preset host-test
   cmake --build --preset host-test

   # Run all host tests with ctest
   ctest --preset host-test --output-on-failure

   # Or filter by subsystem label
   ctest --preset host-test -L "tlb" --output-on-failure
   ```

3. **Running a Single Focused Binary Directly**:
   ```bash
   ./build/host-test/quality/tests/host/test_tlb_shootdown
   ./build/host-test/quality/tests/host/test_pmm_pcache
   ```

## Authoring New Host Tests

When adding a host test for new or modified functionality:
1. Place source file in `quality/tests/host/test_<subsystem>.c`.
2. Register the executable in `quality/tests/host/CMakeLists.txt`:
   ```cmake
   add_executable(test_mysubsys test_mysubsys.c ${SOURCES})
   target_compile_definitions(test_mysubsys PRIVATE BHARAT_HOST_TEST=1)
   add_test(NAME host_test_mysubsys COMMAND test_mysubsys)
   set_tests_properties(host_test_mysubsys PROPERTIES LABELS "mysubsys;core")
   ```
3. Ensure the test is deterministic: no arbitrary sleep loops, no external network or file dependencies, clean teardown of allocated mock memory.
