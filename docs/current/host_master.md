# Host Tests Architecture & Master Document

This document outlines the architecture, purpose, and rules for writing and maintaining host tests in Bharat-OS. Host tests are crucial for verifying core kernel mechanisms, distributed services, and platform policies in a fast, purely software-simulated environment without requiring full emulation (like QEMU) or hardware bring-up.

## Architecture and Purpose

Host tests compile kernel and subsystem source files natively for the host environment (e.g. Linux x86_64, macOS). They link against real subsystem components while isolating hardware dependencies using a thin layer of mock platform stubs.

**Primary Goals:**
1. **Speed & CI Integration:** Execute thousands of subsystem invariants in milliseconds.
2. **Logic Verification:** Test purely algorithmic and policy logic (like scheduling algorithms, memory tracking, capability revocation boundaries) separated from hardware fault handling.
3. **End-to-End Mocking:** Provide highly controlled fault injection (e.g. failing a memory allocation or a generic TLB flush to see how the subsystem recovers).

## Mocks and Stub Organization

Hardware and architecture-specific routines are mocked out by linking against `host_stubs.c` or localized stub files.

**Rules of Thumb:**
- **Authoritative Stubs:** Reusable, basic execution environment mocks (like `hal_cpu_get_id()`, `kernel_panic()`, `hal_ipi_send()`, `mk_send_message()`) reside in `tests/host/host_stubs.c`. This is the single source of truth for host-layer execution invariants.
- **Do Not Scatter Mocks:** Do not define weak symbols or stubs directly inside individual test files if the mock logic is generic. Centralize them to prevent linker collisions or test logic drift.

## Real Logic vs Compatibility Glue

When building a host test, the CMake configuration explicitly distinguishes between the **real logic under test** and the **compatibility glue (stubs)**.

For example, a PMM (Physical Memory Manager) test:
- **Links Real Objects:** `pmm.c`, `pmm_pcache.c`, `numa.c`.
- **Links Shims:** `host_stubs.c` (to mock CPU IDs) and `test_pmm_pcache.c` (to inject fake physical RAM arrays).

*Warning:* Never mock out a function that resides within the primary subsystem boundary you are testing. If you are testing the scheduler, `sched_enqueue` must be real; but the underlying `hal_cpu_disable_interrupts` must be a shim.

## Common Failure Classes

When host tests break, they typically fall into one of these categories:

### 1. Missing Symbols at Link Time
Occurs when a new hardware/HAL dependency is added to a core kernel file, but the `host_stubs.c` or test `CMakeLists.txt` is not updated.
*Fix:* Identify the missing symbol and add it to the authoritative `host_stubs.c` layer, returning a safe default value.

### 2. Stale Invariants After Subsystem Refactors
Occurs when the internal contracts of a subsystem change (e.g. `runnable_count` tracking in the scheduler), but the test setup still mimics the old state.
*Fix:* Do not blindly weaken the assertion. Update the test setup to accurately reflect the new invariant. (e.g., if a thread is automatically enqueued upon creation, the test must gracefully detach it before verifying explicit re-enqueue operations).

### 3. Recursion in Memory/String Shims
Occurs when fallback memory ops (`memops_scalar.c`) inadvertently recursively invoke themselves (like a fake `zero_page` looping via `memset`).
*Fix:* Use Tier-0 non-recursive memory primitives like `arch_memset_raw()` to break the cycle when building host implementations, enforcing strict one-way downward dependencies.

## Rules for Adding New Host Tests

1. **Start with the Contract:** Determine exactly which subsystem contract is being tested.
2. **Isolate Dependencies:** Include the minimal set of real kernel `.c` files required.
3. **Use Unified Stubs:** Link against `tests/host/host_stubs.c`.
4. **Mock Hardware, Not Logic:** Use `active_hal_pt` or `active_mmu` function pointer tables to intercept hardware actions.

## 🛑 Do Not Do This

- **Do not hide real regressions behind dummy stubs.** If a test starts failing because an API contract changed, update the test's assertions, do not just change the stub to bypass the check.
- **Do not duplicate fallback symbols.** Avoid redefining `__attribute__((weak))` symbols across multiple test files. Use `host_stubs.c`.
- **Do not let arch shims call back into generic libc wrappers.** For example, `arch_memset_scalar` must never call `memset`. Always utilize raw `volatile` pointer loops or explicit compiler intrinsics designed for Tier-0 operations.

---
*Generated for Bharat-OS Architecture Documentation.*
